#!/usr/bin/env python3
"""Reproduce VUC original-offset PARTIAL byte matches without accepting them.

Read the existing selected-minus-exact experiment list. Only an original-size
function with every relocation resolvable and all candidate literal bytes
authenticated may receive an exact-byte count. Wrong-size or unvalidated
candidates are NOT partial matches.

The optional checkpoint manifest stores measured lower bounds between sessions;
it is never a whole-ELF score and never contributes to exact acceptance.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
CONFIG = ROOT / "vuc/config"
SELECTED = CONFIG / "selected_candidates.json"
EXACT = CONFIG / "exact_candidates.json"
DEFAULT_BASELINE = CONFIG / "partial_byte_checkpoints.json"

spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)


def read_manifest(path):
    items = json.loads(Path(path).read_text())
    if not isinstance(items, list) or not all(isinstance(x, str) for x in items):
        raise ValueError("expected a JSON list of manifest paths")
    if len(items) != len(set(items)):
        raise ValueError("duplicate manifest paths")
    return items


def checked_path(path):
    if not isinstance(path, str):
        raise ValueError("relative path must be a string")
    full = (ROOT / path).resolve()
    if not full.is_relative_to(ROOT):
        raise ValueError("manifest path escapes repository: " + path)
    return full


def diagnostic(compiler, output, entries, accepted, compare=matcher.run):
    """Use only the real historical compiler and strict original-ELF comparator."""
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    if not isinstance(entries, list) or len(entries) != len(set(entries)):
        raise ValueError("duplicate experiment paths")
    if set(entries) & set(accepted):
        raise ValueError("exact manifest included in partial-only scoring")
    compiled = {}
    seen = set()
    results = []
    for path in entries:
        config_file = checked_path(path)
        config = json.loads(config_file.read_text())
        name, address = config["function"], config["address"]
        identity = (name, int(address, 0))
        if identity in seen:
            raise ValueError("duplicate original function identity: " + name)
        seen.add(identity)
        source = checked_path(config["source"])
        if not config["source"].startswith("vuc/probes/"):
            raise ValueError("partial source must stay in vuc/probes: " + str(source))
        flags = config.get("flags", ["-O2"])
        if not isinstance(flags, list) or not flags or not all(
                isinstance(x, str) and x.startswith("-") for x in flags):
            raise ValueError("invalid historical compiler arguments: " + path)
        code_sha = hashlib.sha256(source.read_bytes()).hexdigest()
        row = {"manifest": path, "function": name, "address": address,
               "original_size": config["size"], "source": config["source"],
               "source_sha256": code_sha, "flags": flags}
        recipe = (str(source), tuple(flags))
        if recipe not in compiled:
            digest = hashlib.sha256(("\0".join([str(source), *flags])).encode()).hexdigest()[:12]
            obj = output / (source.stem + "-" + digest + ".o")
            invocation = subprocess.run([str(compiler), *flags, "-c",
                                         str(source), "-o", str(obj)],
                                        capture_output=True, text=True)
            compiled[recipe] = (obj if invocation.returncode == 0 else None,
                                invocation.stderr[-900:] if invocation.returncode else None)
        obj, compile_error = compiled[recipe]
        if obj is None:
            row.update(status="COMPILE_FAILED", error=compile_error)
        else:
            try:
                score = compare(obj, config_file)
                if score["length"] != config["size"]:
                    raise ValueError("comparator did not return configured original size")
                row.update(status="EXACT_READY" if score["exact"] else "PARTIAL",
                           exact_bytes=score["exact_bytes"],
                           relocations=score["relocations"],
                           verified_literal_bytes=score["verified_literal_bytes"],
                           different_offsets=score["different_offsets"])
            except (OSError, ValueError, KeyError) as exc:
                row.update(status="UNVERIFIED", error=str(exc))
        results.append(row)
    return results


def checkpoint_rows(rows):
    """Only accept original-size, fully authenticated partials as checkpoints."""
    return {r["manifest"]: {
                "function": r["function"], "address": r["address"],
                "original_size": r["original_size"],
                "exact_bytes_floor": r["exact_bytes"],
                "relocations": r["relocations"],
                "verified_literal_bytes": r["verified_literal_bytes"],
            }
            for r in rows if r["status"] in ("PARTIAL", "EXACT_READY")}


def check_baseline(rows, baseline):
    """A previously protected partial cannot silently regress or disappear."""
    found = {r["manifest"]: r for r in rows}
    failures = []
    for manifest, required in baseline.items():
        item = found.get(manifest)
        if item is None:
            failures.append(manifest + ": missing checkpoint")
        elif item["status"] not in ("PARTIAL", "EXACT_READY"):
            failures.append(manifest + ": no longer valid (" + item["status"] + ")")
        else:
            for key, expected in (("address", required["address"]),
                                  ("original_size", required["original_size"]),
                                  ("function", required["function"]),
                                  ("relocations", required["relocations"]),
                                  ("verified_literal_bytes", required["verified_literal_bytes"])):
                if item[key] != expected:
                    failures.append(manifest + ": authenticated " + key + " changed")
            if item["exact_bytes"] < required["exact_bytes_floor"]:
                failures.append(manifest + ": " + str(item["exact_bytes"]) + " fell below " +
                                str(required["exact_bytes_floor"]) + " original-offset bytes")
    return failures


def improvements(rows, baseline):
    """Track verified original-offset byte gains without assuming whole-ELF gains."""
    gains = []
    for row in rows:
        if row["status"] not in ("PARTIAL", "EXACT_READY"):
            continue
        old = baseline.get(row["manifest"])
        previous = old["exact_bytes_floor"] if old else 0
        if row["exact_bytes"] > previous:
            gains.append({"manifest": row["manifest"],
                          "previous_exact_bytes": previous,
                          "current_exact_bytes": row["exact_bytes"],
                          "improvement": row["exact_bytes"] - previous})
    return gains


def summarize(rows, errors, gains=None):
    verified = [r for r in rows if r["status"] in ("PARTIAL", "EXACT_READY")]
    return {"scope": "selected_nonexact_original_function_experiments",
            "whole_binary_score": None, "exact_acceptance": False,
            "experiments": len(rows), "verified_original_size_partials": len(verified),
            "unverified": len(rows) - len(verified),
            "candidate_becoming_exact": sum(r["status"] == "EXACT_READY" for r in rows),
            "verified_partial_bytes": sum(r["exact_bytes"] for r in verified),
            "verified_original_function_bytes": sum(r["original_size"] for r in verified),
            "regressions": errors, "improvements": gains if gains is not None else [],
            "results": rows}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--compiler", required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--baseline", type=Path, default=DEFAULT_BASELINE)
    p.add_argument("--refresh-baseline", action="store_true",
                   help="explicitly replace measured partial floors after reviewing results")
    p.add_argument("--manifest", type=Path, default=SELECTED,
                   help="superset to diagnose; exact functions are excluded")
    args = p.parse_args()

    selected = read_manifest(args.manifest)
    exact = set(read_manifest(EXACT))
    partials = [path for path in selected if path not in exact]
    rows = diagnostic(args.compiler, args.out, partials, exact)
    measured = checkpoint_rows(rows)
    if not args.baseline.exists() and not args.refresh_baseline:
        raise ValueError("no checkpoint baseline: use --refresh-baseline after review")
    previous = json.loads(args.baseline.read_text()) if args.baseline.exists() else {}
    errors = check_baseline(rows, previous)
    gains = improvements(rows, previous)
    # Updating floors requires explicit opt-in; never silently erase a partial
    # that became wrong-sized, lost a relocation or lost exact bytes.
    if args.refresh_baseline and not errors:
        args.baseline.write_text(json.dumps(measured, indent=2) + "\n")
    report = summarize(rows, errors, gains)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "partial_checkpoint_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("%d/%d original-length, fully authenticated partial experiments; %d/%d "
          "original-offset bytes within ONLY those experiments" %
          (report["verified_original_size_partials"], report["experiments"],
           report["verified_partial_bytes"], report["verified_original_function_bytes"]))
    print("Not exact acceptance. Unverified: %d. Regression failures: %d." %
          (report["unverified"], len(errors)))
    for gain in gains:
        print("IMPROVED +%d: %s: %d -> %d original-offset bytes" %
              (gain["improvement"], gain["manifest"],
               gain["previous_exact_bytes"], gain["current_exact_bytes"]))
    for row in rows:
        score = ("%d/%d" % (row["exact_bytes"], row["original_size"])
                 if "exact_bytes" in row else row["status"])
        print("%-14s %-9s %s" % (row["address"], score, row["manifest"]))
    for error in errors:
        print("REGRESSION: " + error, file=sys.stderr)
    print("Whole-binary score: UNAVAILABLE")
    return 1 if errors else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError) as exc:
        print("partial-checkpoints: " + str(exc), file=sys.stderr)
        sys.exit(1)

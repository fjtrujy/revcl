#!/usr/bin/env python3
"""Strictly recheck source/config probes that are not in the accepted manifest.

Only compile and compare. Never modify the repository or promote partial
functions. Results are diagnostics, NOT an executable-wide matching score.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)


def scan(compiler, output, config_paths):
    output.mkdir(parents=True, exist_ok=True)
    accepted = set(json.loads((ROOT / "vuc/config/exact_candidates.json").read_text()))
    accepted_identities = {
        (item["function"], int(item["address"], 0))
        for item in (json.loads((ROOT / path).read_text()) for path in accepted)
    }
    compiled = {}
    rows = []
    for config_path in config_paths:
        rel = config_path.relative_to(ROOT).as_posix()
        if rel in accepted:
            continue
        try:
            cfg = json.loads(config_path.read_text())
            if (cfg["function"], int(cfg["address"], 0)) in accepted_identities:
                # Alternate configs for the same accepted original function
                # are NOT new byte-exact discoveries.
                continue
            source = (ROOT / cfg["source"]).resolve()
            flags = cfg.get("flags", ["-O2", "-frename-registers"])
            if not source.is_relative_to(ROOT) or not source.is_file():
                raise ValueError("source not found inside repository")
            key = (source, tuple(flags))
            if key not in compiled:
                ident = str(source) + "\0" + "\0".join(flags)
                obj = output / (source.stem + "-" + hashlib.sha256(ident.encode()).hexdigest()[:12] + ".o")
                command = [compiler, *flags, "-c", str(source), "-o", str(obj)]
                run = subprocess.run(command, text=True, capture_output=True)
                compiled[key] = (obj, run)
            obj, run = compiled[key]
            if run.returncode:
                raise ValueError("compiler: " + run.stderr[-280:].replace("\n", " "))
            result = matcher.run(obj, config_path)
            rows.append({"config": rel, "address": cfg["address"],
                         "source": cfg["source"], "size": cfg["size"],
                         "exact_bytes": result["exact_bytes"],
                         "exact": result["exact"],
                         "relocations": result["relocations"]})
        except (ValueError, KeyError, RuntimeError) as e:
            rows.append({"config": rel, "error": str(e)[:380]})
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--configs", nargs="*", type=Path,
                        help="default: all configs with *probe.json in vuc/config")
    args = parser.parse_args()
    paths = args.configs or sorted((ROOT / "vuc/config").glob("*probe.json"))
    paths = [(p if p.is_absolute() else ROOT / p).resolve() for p in paths]
    if any(not p.is_relative_to(ROOT / "vuc/config") for p in paths):
        parser.error("all configs must be within vuc/config")
    rows = scan(args.compiler, args.out, paths)
    (args.out / "pending_probe_results.json").write_text(json.dumps(rows, indent=2) + "\n")
    for row in sorted(rows, key=lambda x: (not x.get("exact", False),
                                         -(x.get("exact_bytes") or 0))):
        if "error" in row:
            print("UNAVAILABLE", row["config"], row["error"])
        else:
            print("EXACT" if row["exact"] else "PARTIAL",
                  f'{row["exact_bytes"]}/{row["size"]}',
                  row["config"])
    print(f'Checked {len(rows)} pending configurations; '
          f'{sum(r.get("exact", False) for r in rows)} fully exact unaccepted symbols.')


if __name__ == "__main__":
    main()

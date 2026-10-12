#!/usr/bin/env python3
"""Compile and strictly verify named initialized VUC data objects.

This is a separate, explicit original-ELF data-object acceptance gate.
It does not assert data-section layout, ELF linkage, or whole-binary equality.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
EXACT_MANIFEST = ROOT / "vuc/config/exact_data_candidates.json"
spec = importlib.util.spec_from_file_location("vuc_match_data", ROOT/"vuc/scripts/match_data.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)


def within_repo(relative):
    candidate = (ROOT / relative).resolve()
    if not candidate.is_relative_to(ROOT):
        raise ValueError("candidate path escapes repository: " + str(relative))
    return candidate


def run(compiler, out, manifests):
    if not isinstance(manifests, list) or not manifests:
        raise ValueError("expected a nonempty list of data object configs")
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    seen = set()
    built = {}
    results = []

    for relative in manifests:
        config_path = within_repo(relative)
        config = json.loads(config_path.read_text())
        symbol = config["symbol"]
        identity = (symbol, int(config["address"], 0))
        if identity in seen:
            raise ValueError("duplicate original data object: " + symbol)
        seen.add(identity)
        source = within_repo(config["source"])
        if not source.is_relative_to(ROOT / "vuc/src"):
            raise ValueError("accepted data source must be under vuc/src: " + str(source))
        flags = config["flags"]
        if (not isinstance(flags, list) or not flags
                or not all(isinstance(f, str) and f.startswith("-") for f in flags)):
            raise ValueError("invalid compiler flags for " + symbol)
        key = (str(source), tuple(flags))
        if key not in built:
            hash_input = str(source) + "\0" + "\0".join(flags)
            object_file = out / (source.stem + "-" +
                                 hashlib.sha256(hash_input.encode()).hexdigest()[:10] + ".o")
            command = [str(compiler), *flags, "-c", str(source), "-o", str(object_file)]
            invocation = subprocess.run(command, capture_output=True, text=True)
            if invocation.returncode:
                raise RuntimeError("failed to compile " + str(source) + ": " + invocation.stderr[-1600:])
            built[key] = object_file
        result = matcher.run(built[key], config_path)
        result["source"] = config["source"]
        result["candidate_flags"] = list(flags)
        results.append(result)

    exact = all(item["exact"] for item in results)
    summary = {
        "scope": "explicit_original_named_data_objects_only",
        "whole_binary_score": None,
        "whole_binary_byte_exact": False,
        "acceptance": "PASS" if exact else "FAIL",
        "all_candidates_exact": exact,
        "compiler": str(compiler),
        "data_objects_checked": len(results),
        "fully_exact_data_objects": sum(item["exact"] for item in results),
        "checked_data_bytes": sum(item["size"] for item in results),
        "matching_data_bytes": sum(item["exact_bytes"] for item in results),
        "resolved_relocations": sum(item["resolved_relocations"] for item in results),
        "results": results,
    }
    (out / "data_candidate_summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    return summary


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, help="historical GCC executable")
    parser.add_argument("--out", required=True, help="directory for compiled objects and report")
    parser.add_argument("--manifest", default=str(EXACT_MANIFEST))
    args = parser.parse_args(argv)
    summary = run(args.compiler, args.out, json.loads(Path(args.manifest).read_text()))
    print("%d/%d exact named original data objects; %d/%d verified bytes" %
          (summary["fully_exact_data_objects"], summary["data_objects_checked"],
           summary["matching_data_bytes"], summary["checked_data_bytes"]))
    print("Named-data byte-exact acceptance: " + summary["acceptance"])
    print("Whole-binary score: UNAVAILABLE")
    return 0 if summary["all_candidates_exact"] else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, RuntimeError) as exc:
        print("data-candidate-suite: " + str(exc), file=sys.stderr)
        sys.exit(2)

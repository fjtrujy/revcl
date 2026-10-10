#!/usr/bin/env python3
"""Audit explicitly configured VUC function hypotheses without claiming whole-file score."""
import argparse
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_match_probe", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)


def audit(jobs):
    if not isinstance(jobs, list) or not jobs:
        raise ValueError("expected a nonempty list of function jobs")
    results = []
    seen = set()
    for job in jobs:
        config, obj = job["config"], job["object"]
        result = matcher.run(obj, config)
        key = result["function"]
        if key in seen:
            raise ValueError("duplicate VUC function: " + key)
        seen.add(key)
        results.append(result)
    total = sum(r["length"] for r in results)
    exact = sum(r["exact_bytes"] for r in results)
    return {"scope": "explicit_candidate_functions_only",
            "whole_binary_score": None,
            "functions_checked": len(results),
            "fully_exact_functions": sum(r["exact"] for r in results),
            "checked_code_bytes": total,
            "matching_code_bytes": exact,
            "checked_literal_bytes": sum(r["verified_literal_bytes"] for r in results),
            "results": results}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", required=True, help="JSON list of {config, object} entries")
    args = parser.parse_args()
    manifest = json.loads(Path(args.jobs).read_text())
    print(json.dumps(audit(manifest), indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError) as exc:
        print("vuc-audit:", exc, file=sys.stderr)
        sys.exit(1)

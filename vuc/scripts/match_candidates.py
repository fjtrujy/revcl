#!/usr/bin/env python3
"""Compile and audit a curated set of VUC function hypotheses.

A candidate suite reports only verified named functions, never whole-binary
coverage. All objects and reports are written under an explicit scratch path.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = ROOT / "vuc/config/selected_candidates.json"
spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)


def within_repo(relative):
    candidate = (ROOT / relative).resolve()
    if not candidate.is_relative_to(ROOT):
        raise ValueError("candidate path escapes repository: " + str(relative))
    return candidate


def run(compiler, out, manifests, options=None):
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    if not isinstance(manifests, list) or not manifests:
        raise ValueError("expected a nonempty array of manifest paths")
    flags = ["-O2"] if options is None else list(options)
    compiled = {}
    results = []
    seen = set()
    for relative in manifests:
        config_path = within_repo(relative)
        config = json.loads(config_path.read_text())
        name = config["function"]
        if name in seen:
            raise ValueError("duplicate function: " + name)
        seen.add(name)
        source = within_repo(config["source"])
        key = str(source)
        if key not in compiled:
            # Hash full source path to avoid collisions between equal filenames.
            obj = out / (source.stem + "-" + hashlib.sha256(key.encode()).hexdigest()[:10] + ".o")
            command = [str(compiler), *flags, "-c", str(source), "-o", str(obj)]
            invocation = subprocess.run(command, capture_output=True, text=True)
            if invocation.returncode:
                raise RuntimeError("compilation failed for " + str(source) + ": " +
                                   invocation.stderr[-1600:])
            compiled[key] = obj
        result = matcher.run(compiled[key], config_path)
        result["source"] = config["source"]
        results.append(result)

    summary = {
        "scope": "explicit_candidate_functions_only",
        "whole_binary_score": None,
        "compiler": str(compiler),
        "flags": flags,
        "functions_checked": len(results),
        "fully_exact_functions": sum(r["exact"] for r in results),
        "matching_code_bytes": sum(r["exact_bytes"] for r in results),
        "checked_code_bytes": sum(r["length"] for r in results),
        "checked_literal_bytes_per_probe": sum(r["verified_literal_bytes"] for r in results),
        "results": results,
    }
    (out / "candidate_summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    return summary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True, help="explicit historic C++ wrapper")
    parser.add_argument("--out", required=True, help="scratch directory for compiled objects/results")
    parser.add_argument("--manifest", default=str(DEFAULT_MANIFEST))
    args = parser.parse_args()
    entries = json.loads(Path(args.manifest).read_text())
    result = run(args.compiler, args.out, entries)
    print("%d/%d exact candidate functions; %d/%d matching bytes in checked functions" %
          (result["fully_exact_functions"], result["functions_checked"],
           result["matching_code_bytes"], result["checked_code_bytes"]))
    print("Whole-binary score: UNAVAILABLE")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, RuntimeError) as exc:
        print("candidate-suite: " + str(exc), file=sys.stderr)
        sys.exit(1)

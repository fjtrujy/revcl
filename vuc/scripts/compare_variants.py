#!/usr/bin/env python3
"""Compare historical-compiler optimization variants against one VUC function.

All scores are candidate-scoped; this command does not estimate whole-file
matching. Use only with an explicit compiler path and an evidence-backed
function/relocation manifest.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)

VARIANTS = {
    "O0": ["-O0"],
    "O1": ["-O1"],
    "O2": ["-O2"],
    "O2-rename": ["-O2", "-frename-registers"],
    "O2-no-sched1": ["-O2", "-fno-schedule-insns"],
    "O2-no-sched2": ["-O2", "-fno-schedule-insns2"],
    "O2-no-gcse": ["-O2", "-fno-gcse"],
    "O2-no-peephole2": ["-O2", "-fno-peephole2"],
    "O2-no-reorder": ["-O2", "-fno-reorder-blocks"],
    "O2-no-strict-alias": ["-O2", "-fno-strict-aliasing"],
    "O2-i386": ["-O2", "-march=i386"],
    "O2-i486": ["-O2", "-march=i486"],
    "O2-i686": ["-O2", "-march=i686"],
    "O2-pentium": ["-O2", "-march=pentium"],
    "O2-tune-pentium3": ["-O2", "-mtune=pentium3"],
    "Os": ["-Os"],
}


def run(compiler, output, source, config, variants=VARIANTS):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    records = {}
    for name, flags in variants.items():
        obj = output / (name + ".o")
        command = [compiler, *flags, "-c", str(source), "-o", str(obj)]
        result = subprocess.run(command, capture_output=True, text=True)
        record = {"flags": flags, "compile_exit": result.returncode}
        if result.returncode:
            record["error"] = result.stderr[-600:]
        else:
            try:
                record.update(matcher.run(obj, config))
            except (ValueError, KeyError, OSError) as exc:
                record["error"] = str(exc)
        records[name] = record
    report = {"scope": "single_VUC_function_hypothesis",
              "whole_binary_score": None,
              "compiler": compiler, "config": str(config),
              "results": records}
    (output / "comparison.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument("--source", default=str(ROOT / "vuc/probes/asm_output_hypothesis.cpp"))
    parser.add_argument("--config", default=str(ROOT / "vuc/config/asm_output_jan25_probe.json"))
    args = parser.parse_args()
    report = run(args.compiler, args.out, args.source, args.config)
    for name, record in report["results"].items():
        metric = ("%d/%d" % (record["exact_bytes"], record["length"])
                  if "exact_bytes" in record else "UNAVAILABLE")
        print("%-20s %s %s" % (name, metric, record.get("error", "")))
    print("Whole-binary score: UNAVAILABLE")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError) as exc:
        print("compare-variants:", exc, file=sys.stderr)
        sys.exit(1)

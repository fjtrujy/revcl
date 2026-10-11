#!/usr/bin/env python3
"""Check GCC 3.3.3 compiler flag hypotheses for VUC script_error()."""
import importlib.util
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215")
spec = importlib.util.spec_from_file_location(
    "vuc_match", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
config = ROOT / "vuc/config/script_error_probe.json"
source = ROOT / "vuc/probes/script_error.cpp"

choices = [
    "", "-fno-schedule-insns2", "-fno-schedule-insns",
    "-fno-gcse", "-fno-cse-follow-jumps", "-fno-cse-skip-blocks",
    "-fno-regmove", "-fno-peephole2", "-fno-reorder-blocks",
    "-fno-if-conversion", "-fno-if-conversion2", "-fno-web",
    "-fno-strict-aliasing", "-fno-guess-branch-probability",
    "-fno-unit-at-a-time", "-fno-expensive-optimizations",
    "-fno-rerun-cse-after-loop", "-fno-delayed-branch",
    "-fno-omit-frame-pointer", "-fno-optimize-sibling-calls",
    "-fno-inline", "-fno-builtin", "-falign-labels=1",
    "-falign-loops=1", "-falign-functions=1", "-falign-jumps=2",
    "-falign-jumps=3", "-falign-jumps=4", "-march=i386",
    "-march=i486", "-march=i586", "-march=i686", "-mtune=i386",
    "-mtune=i486", "-mtune=i686",
]
results = []
for compiler_name in ("g++-jan25-as215", "g++-dec06-as215"):
    compiler = SCRATCH / compiler_name
    for index, extra in enumerate(choices):
        flags = ["-O2", "-frename-registers", "-falign-jumps=1"]
        if extra:
            flags.append(extra)
        obj = SCRATCH / f"script-error-flags-{compiler_name}-{index:02d}.o"
        run = subprocess.run([str(compiler), *flags, "-c", str(source),
                              "-o", str(obj)], capture_output=True, text=True)
        if run.returncode:
            results.append((0, compiler_name, extra, run.stderr[-90:]))
            continue
        try:
            score = matcher.run(obj, config)
            results.append((score["exact_bytes"], compiler_name, extra,
                            str(score["length"]), score["different_offsets"]))
            if score["exact"]:
                print("NEW EXACT", compiler_name, extra, flush=True)
        except ValueError as exc:
            results.append((0, compiler_name, extra, str(exc)[:80]))
for item in sorted(results, reverse=True)[:26]:
    print(*item)

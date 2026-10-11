#!/usr/bin/env python3
"""Pairwise period-plausible GCC 3.3 flags for Value integer multiply register.

No assembly patches or register binding pragmas. A candidate counts only if
the canonical relocation-aware byte-exact matcher accepts the full function.
"""
import importlib.util
import itertools
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
TEMP = Path("/tmp/vuc-binutils215")
spec = importlib.util.spec_from_file_location("vucmatch", ROOT / "vuc/scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
source = ROOT / "vuc/probes/value_ml.cpp"
config = ROOT / "vuc/config/value_ml_probe.json"
compiler = TEMP / "g++-jan25-as215"
extra = (
    "-fno-gcse", "-fno-web", "-march=i386", "-fno-schedule-insns2",
    "-fno-strict-aliasing", "-mtune=i686", "-fno-cse-follow-jumps",
    "-fno-reorder-blocks", "-fno-unit-at-a-time", "-fno-peephole2",
    "-fno-regmove", "-fno-if-conversion", "-fno-guess-branch-probability",
)
base = ("-O2", "-frename-registers", "-falign-jumps=1")
rows = []
for i, pair in enumerate(itertools.combinations(extra, 2)):
    obj = TEMP / ("vmul-flag-pairs-%03d.o" % i)
    command = [str(compiler), *base, *pair, "-c", str(source), "-o", str(obj)]
    proc = subprocess.run(command, capture_output=True, text=True)
    if proc.returncode:
        rows.append((0, pair, proc.stderr[-110:]))
        continue
    try:
        score = m.run(obj, config)
        rows.append((score["exact_bytes"], pair, str(score["length"]),
                     str(score["different_offsets"])))
        if score["exact"]:
            print("NEW EXACT", pair, flush=True)
            break
    except ValueError as exc:
        rows.append((0, pair, str(exc)[:100]))
for result in sorted(rows, reverse=True)[:25]:
    print(*result)

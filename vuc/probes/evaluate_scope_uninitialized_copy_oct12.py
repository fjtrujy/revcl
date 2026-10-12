#!/usr/bin/env python3
"""Test historically supported GCC 3.3 loop/branch alignment for scope STL copy."""
import importlib.util
import itertools
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/scope-uninit-oct12")
TMP.mkdir(parents=True, exist_ok=True)
SPEC = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
MATCHER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MATCHER)
SRC = ROOT / "vuc/probes/scope_uninitialized_copy.cpp"
CONFIG = ROOT / "vuc/config/scope_uninitialized_copy_probe.json"
COMPILER = "/tmp/vuc-binutils215/g++-jan25-as215"
results = []
for rename, loop, jump, label in itertools.product((False, True), (1,16), (1,2,4,8,16), (1,16)):
    flags = ["-O2", "-falign-loops=%d" % loop,
             "-falign-jumps=%d" % jump, "-falign-labels=%d" % label]
    if rename: flags.append("-frename-registers")
    obj = TMP / "candidate.o"
    cp = subprocess.run([COMPILER, *flags, "-c", str(SRC), "-o", str(obj)], capture_output=True, text=True)
    if cp.returncode:
        results.append((0, 0, flags, "compile failed"))
        continue
    try:
        r = MATCHER.run(obj, CONFIG)
        results.append((r["exact_bytes"], r["length"], flags, "same size"))
        if r["exact"]:
            print("FULL EXACT MATCH", flags)
            raise SystemExit(0)
    except ValueError as exc:
        results.append((0, 0, flags, str(exc)[:85]))
for exact, length, flags, status in sorted(results, reverse=True)[:25]:
    print(exact, "/", length, " ".join(flags), status)
print("Checked", len(results), "compiler variants")

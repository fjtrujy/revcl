#!/usr/bin/env python3
"""Bounded GCC 3.3.3 register-pass controls for ASTFunCall printer.

The historical original differs by two register-encoding bytes under -O2.
Check only period-plausible optimizer/code-generation controls and preserve
the original Printer_UglyPrint method and full literal-set authentication.
"""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/printer-two-registers-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
source = ROOT / "vuc/src/Printer.cpp"
flags = {
    "plain": [],
    "plain-no-gcse": ["-fno-gcse"],
    "plain-no-regmove": ["-fno-regmove"],
    "plain-no-sched2": ["-fno-schedule-insns2"],
    "plain-no-sched": ["-fno-schedule-insns"],
    "plain-no-peephole2": ["-fno-peephole2"],
    "plain-no-strictalias": ["-fno-strict-aliasing"],
    "plain-no-expensive": ["-fno-expensive-optimizations"],
    "plain-no-reorder": ["-fno-reorder-blocks"],
    "plain-no-ifconv": ["-fno-if-conversion"],
    "plain-no-cse": ["-fno-cse-follow-jumps"],
    "plain-no-rerun-cse": ["-fno-rerun-cse-after-loop"],
    "plain-no-guess": ["-fno-guess-branch-probability"],
    "plain-no-inline": ["-fno-inline"],
    "plain-i386": ["-march=i386"],
    "plain-i486": ["-march=i486"],
    "plain-i586": ["-march=i586"],
    "plain-align-j1": ["-falign-jumps=1"],
    "plain-align-j4": ["-falign-jumps=4"],
    "plain-align-l1": ["-falign-labels=1"],
    "plain-align-l4": ["-falign-labels=4"],
    "plain-align-functions4": ["-falign-functions=4"],
    "plain-function-sections": ["-ffunction-sections"],
    "plain-fuse-cxa": ["-fuse-cxa-atexit"],
    "rename": ["-frename-registers"],
    "rename-i386": ["-frename-registers", "-march=i386"],
    "rename-no-sched2": ["-frename-registers", "-fno-schedule-insns2"],
}
for label, extras in flags.items():
    obj = SCRATCH / (label + ".o")
    build = subprocess.run(
        ["/tmp/vuc-binutils215/g++-jan25-as215", "-O2", *extras,
         "-c", str(source), "-o", str(obj)],
        capture_output=True, text=True)
    if build.returncode:
        print(label, "COMPILE_FAIL", build.stderr[-130:], flush=True)
        continue
    outcome = []
    for function, config in [
        ("call", "print_ast_fun_context_oct14_probe.json"),
        ("ugly", "ugly_printer_unit_oct13_probe.json"),
    ]:
        try:
            result = matcher.run(obj, ROOT / "vuc/config" / config)
            outcome.append("%s=%d/%d" % (function, result["exact_bytes"], result["length"]))
            if function == "call" and result["exact"]:
                print("NEW_EXACT", label, str(obj), flush=True)
        except ValueError as error:
            outcome.append(function + "=REJECT " + str(error)[:70])
    print(label, *outcome, flush=True)

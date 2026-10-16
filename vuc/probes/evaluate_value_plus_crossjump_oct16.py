#!/usr/bin/env python3
"""Original GCC 3.3 cross-jump diagnostic for Value addition's x87 tail.

The target performs a distinct scalar case-3 add and four case-4 lanes.
Original assembly keeps case-3 and lane 0 separate. Test whether a
historically supported optimizer control recovers this without byte edits.
"""
from pathlib import Path
import importlib.util
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-value-plus-crossjump-oct16")
O.mkdir(exist_ok=True)
original = (R / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
old = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
forms = {
    "selected": original,
    "ascending": original.replace(old, """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""),
}
flags = {
    "no-crossjump": ["-fno-crossjumping"],
    "no-gcse": ["-fno-gcse"],
    "no-reorder": ["-fno-reorder-blocks"],
    "no-cse-follow": ["-fno-cse-follow-jumps"],
}
spec = importlib.util.spec_from_file_location("match", R / "scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
for form, code in forms.items():
    path = O / (form + ".cpp")
    path.write_text(code)
    for flag, extra in flags.items():
        obj = O / (form + "-" + flag + ".o")
        cmd = ["/tmp/vuc-binutils215/g++-jan25-as215",
               "-O2", "-frename-registers", "-falign-jumps=1",
               *extra, "-c", str(path), "-o", str(obj)]
        p = subprocess.run(cmd, capture_output=True, text=True)
        if p.returncode:
            print("COMPILE_FAIL", form, flag, p.stderr[-120:], flush=True)
            continue
        try:
            d = m.run(obj, R / "config/value_addition_partial_oct14_probe.json")
        except Exception as e:
            print("REJECT", form, flag, str(e)[:180], flush=True)
            continue
        print("RESULT", form, flag, d["exact_bytes"], d["length"],
              d["different_offsets"][:22], flush=True)
        if d["exact"]:
            (O / "EXACT").write_text(form + " " + flag + "\n")
            print("BYTE_EXACT", form, flag, flush=True)

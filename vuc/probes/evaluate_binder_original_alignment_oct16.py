#!/usr/bin/env python3
"""Narrow historical alignment combinations for the original Binder_Bind body.

These controls test GCC-generated CFG boundaries, not handwritten padding.
Only a 552-byte candidate with authenticated symbols could become EXPERIMENTAL.
"""
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-binder-alignment-oct16")
O.mkdir(exist_ok=True)
source = R / "probes/BinderBindASTModuleFreshOct15s.cpp"
settings = {
    "j8-label1-loop1": [8, 1, 1],
    "j8-label2-loop1": [8, 2, 1],
    "j8-label4-loop1": [8, 4, 1],
    "j8-label1-loop2": [8, 1, 2],
    "j8-label1-loop4": [8, 1, 4],
    "j8-label1-loop8": [8, 1, 8],
    "j4-label8-loop1": [4, 8, 1],
    "j4-label8-loop4": [4, 8, 4],
    "j4-label8-loop8": [4, 8, 8],
}
for name, (jump, label, loop) in settings.items():
    obj = O / (name + ".o")
    cmd = ["/tmp/vuc-binutils215/g++-jan25-as215", "-O2",
           "-frename-registers", "-falign-jumps=" + str(jump),
           "-falign-labels=" + str(label),
           "-falign-loops=" + str(loop),
           "-c", str(source), "-o", str(obj)]
    p = subprocess.run(cmd, capture_output=True, text=True)
    if p.returncode:
        print("ERROR", name, p.stderr[-140:], flush=True)
        continue
    n = subprocess.run(["nm", "-S", str(obj)], capture_output=True, text=True)
    found = [int(x.split()[1], 16) for x in n.stdout.splitlines()
             if x.endswith(" T _Z11Binder_BindP9ASTModule")]
    if found:
        print("RESULT", name, found[0],
              "ORIGINAL_LENGTH" if found[0] == 552 else "", flush=True)

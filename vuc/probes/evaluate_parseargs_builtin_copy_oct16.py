#!/usr/bin/env python3
"""Original VUC parser builtin-copy and historical GCC alignment diagnostic.

The source uses a genuine __builtin_memcpy call for the directory prefix;
ordinary string calls are left external with -fno-builtin. Candidate lengths
are reported before any strict instruction-byte eligibility claim.
"""
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-parseargs-builtin-copy-oct16")
O.mkdir(exist_ok=True)
source = (R / "probes/ParseArgumentsOriginalBranchesOct16.cpp").read_text()
source = source.replace(
    "std::memcpy(inputPath, inputFileName, size);",
    "__builtin_memcpy(inputPath, inputFileName, size);")
path = O / "ParseArgumentsBuiltinCopy.cpp"
path.write_text(source)
base = ["-O2", "-frename-registers", "-minline-all-stringops",
        "-fno-builtin", "-falign-jumps=1"]
variants = {
    "j1": [],
    "labels1": ["-falign-labels=1"],
    "loops1": ["-falign-loops=1"],
    "labels1-loops1": ["-falign-labels=1", "-falign-loops=1"],
    "labels2-loops1": ["-falign-labels=2", "-falign-loops=1"],
    "labels4-loops1": ["-falign-labels=4", "-falign-loops=1"],
    "labels1-loops2": ["-falign-labels=1", "-falign-loops=2"],
    "labels1-loops4": ["-falign-labels=1", "-falign-loops=4"],
}
for name, extra in variants.items():
    obj = O / (name + ".o")
    cmd = ["/tmp/vuc-binutils215/g++-jan25-as215",
           *base, *extra, "-c", str(path), "-o", str(obj)]
    done = subprocess.run(cmd, capture_output=True, text=True)
    if done.returncode:
        print("FAIL", name, done.stderr[-130:], flush=True)
        continue
    nm = subprocess.run(["nm", "-S", str(obj)], capture_output=True, text=True)
    sizes = [int(line.split()[1], 16) for line in nm.stdout.splitlines()
             if line.endswith(" T _Z14ParseArgumentsiPPc")]
    if sizes:
        print("RESULT", name, sizes[0], "ORIGINAL_LENGTH" if sizes[0] == 777 else "", flush=True)
    else:
        print("MISSING", name, flush=True)

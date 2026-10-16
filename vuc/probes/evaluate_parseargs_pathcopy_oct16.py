#!/usr/bin/env python3
"""Compile actual C++03 path-prefix copy alternatives for ParseArguments.

Length-only discovery. Do not classify wrong-sized outputs as partial matches.
"""
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-parseargs-pathcopy-oct16")
O.mkdir(exist_ok=True)
base = (R / "probes/ParseArgumentsOriginalBranchesOct16.cpp").read_text()
old = "std::memcpy(inputPath, inputFileName, size);"
forms = {
    "memcpy": "std::memcpy(inputPath, inputFileName, size);",
    "memmove": "std::memmove(inputPath, inputFileName, size);",
    "builtin-memmove": "__builtin_memmove(inputPath, inputFileName, size);",
    "std-copy": "std::copy(inputFileName, inputFileName+size, inputPath);",
    "copy-backward": "std::copy_backward(inputFileName, inputFileName+size, inputPath+size);",
    "char-traits-copy": "std::char_traits<char>::copy(inputPath, inputFileName, size);",
    "char-traits-move": "std::char_traits<char>::move(inputPath, inputFileName, size);",
    "simple-loop": "for(unsigned k=0;k<size;k++) inputPath[k]=inputFileName[k];",
    "loop-iterator": "char *p=inputPath; const char *q=inputFileName; while(size--) *p++=*q++;",
}
for name, replacement in forms.items():
    path = O / (name + ".cpp")
    path.write_text("#include <algorithm>\n" + base.replace(old, replacement))
    for flags_name, flags in {
        "o2": ["-O2", "-frename-registers"],
        "o2align8": ["-O2", "-frename-registers", "-falign-jumps=8"],
    }.items():
        obj = O / (name + "-" + flags_name + ".o")
        result = subprocess.run(
            ["/tmp/vuc-binutils215/g++-jan25-as215", *flags,
             "-c", str(path), "-o", str(obj)],
            text=True, capture_output=True)
        if result.returncode:
            print("FAIL", name, flags_name, result.stderr[-170:], flush=True)
            continue
        nm = subprocess.run(["nm", "-S", str(obj)], text=True, capture_output=True)
        sizes = [int(line.split()[1], 16) for line in nm.stdout.splitlines()
                 if line.endswith(" T _Z14ParseArgumentsiPPc")]
        if not sizes:
            print("MISSING", name, flags_name, flush=True)
            continue
        size = sizes[0]
        print("RESULT", name, flags_name, size, "ORIGINAL_LENGTH" if size==777 else "", flush=True)

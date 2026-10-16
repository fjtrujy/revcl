#!/usr/bin/env python3
"""Test original-era global-array alignment as a cause of ParseArguments copying.

This is a source-level experiment, not an instruction-byte patch. Wrong-length
objects are negative controls and are never promoted to selected candidates.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-parseargs-aligned-copy-oct16")
OUT.mkdir(exist_ok=True)
base = (ROOT / "probes/ParseArgumentsOriginalBranchesOct16.cpp").read_text()
base = base.replace("std::memcpy(inputPath, inputFileName, size);",
                    "__builtin_memcpy(inputPath, inputFileName, size);")
source_forms = {
    "unaligned": base,
    "source4": base.replace("extern char inputFileName[256];",
                            "extern char inputFileName[256] __attribute__((aligned(4)));"),
    "dest4": base.replace("extern char inputPath[256];",
                          "extern char inputPath[256] __attribute__((aligned(4)));"),
    "both4": base.replace("extern char inputFileName[256];",
                          "extern char inputFileName[256] __attribute__((aligned(4)));")
                 .replace("extern char inputPath[256];",
                          "extern char inputPath[256] __attribute__((aligned(4)));"),
    "both8": base.replace("extern char inputFileName[256];",
                          "extern char inputFileName[256] __attribute__((aligned(8)));")
                 .replace("extern char inputPath[256];",
                          "extern char inputPath[256] __attribute__((aligned(8)));"),
    "both16": base.replace("extern char inputFileName[256];",
                           "extern char inputFileName[256] __attribute__((aligned(16)));")
                  .replace("extern char inputPath[256];",
                           "extern char inputPath[256] __attribute__((aligned(16)));"),
}
flags = ["-O2", "-frename-registers", "-minline-all-stringops",
         "-fno-builtin", "-falign-jumps=1", "-falign-labels=1",
         "-falign-loops=4"]
for name, content in source_forms.items():
    path = OUT / (name + ".cpp")
    path.write_text(content)
    for compiler in ("jan25",):
        obj = OUT / (name + "-" + compiler + ".o")
        done = subprocess.run(
            ["/tmp/vuc-binutils215/g++-" + compiler + "-as215",
             *flags, "-c", str(path), "-o", str(obj)],
            capture_output=True, text=True)
        if done.returncode:
            print("FAIL", name, done.stderr[-180:], flush=True)
            continue
        nm = subprocess.run(["nm", "-S", str(obj)], capture_output=True, text=True)
        sizes = [int(line.split()[1], 16) for line in nm.stdout.splitlines()
                 if line.endswith(" T _Z14ParseArgumentsiPPc")]
        if not sizes:
            print("MISSING", name, flush=True)
            continue
        print("RESULT", name, compiler, sizes[0],
              "ORIGINAL_LENGTH" if sizes[0] == 777 else "", flush=True)

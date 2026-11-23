#!/usr/bin/env python3
"""Recover script_error register selection via real VUC compiler unit ordering.

The immutable original uses EAX, ECX, EDX for three successive stream/text
relocation operands. The accepted real VucCompiler TU instead gets EDX, EDX,
ECX. VCL experience suggests real predecessor functions and compiler passes
may move register selection. Compile only original source definitions, in
plausible orders, checking full original length, 15 authentic relocations
and all original literal bytes. Never modify exact source or force registers.
"""
from pathlib import Path
import itertools
import importlib.util
import json
import subprocess
VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-script-unit-order-20261123")
OUT.mkdir(exist_ok=True)
init = (VUC/"src/CurrFileInitialization.cpp").read_text()
lexer = (VUC/"src/ScriptLexer.cpp").read_text()
main = (VUC/"src/Main.cpp").read_text()
unit = (VUC/"src/VucCompilerUnit.cpp").read_text()
error = unit[unit.index("// This diagnostic"):unit.index('#include "Main.cpp"')]
sources={"lexer":lexer, "main":main, "error":error}
spec=importlib.util.spec_from_file_location("m", VUC/"scripts/match_probe.py")
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
cfg=VUC/"config/script_error_probe.json"
flags={
  "baseline": ["-O2","-frename-registers","-falign-jumps=1"],
  "cxa": ["-O2","-frename-registers","-falign-jumps=1","-fuse-cxa-atexit"],
  "sections": ["-O2","-frename-registers","-falign-jumps=1","-ffunction-sections"],
  "cxa-sections": ["-O2","-frename-registers","-falign-jumps=1","-fuse-cxa-atexit","-ffunction-sections"],
  "no-rename": ["-O2","-falign-jumps=1"],
}
rows=[]
for order in itertools.permutations(sources):
    label="-".join(order)
    src=OUT/(label+".cpp")
    src.write_text(init+"\n"+"\n".join(sources[x] for x in order))
    for flag, options in flags.items():
        for compiler in ("jan25", "dec06"):
            obj=OUT/(label+"-"+flag+"-"+compiler+".o")
            p=subprocess.run(["/tmp/vuc-binutils215/g++-"+compiler+"-as215",*options,
                              "-c", str(src),"-o",str(obj)], capture_output=True,text=True)
            row={"order":label,"flag":flag,"compiler":compiler}
            if p.returncode:
                row["error"]=p.stderr[-320:]
            else:
                try:
                    matched=matcher.run(obj,cfg)
                    row.update(score=matched["exact_bytes"], length=matched["length"],
                               differences=matched["different_offsets"],
                               verified_literals=matched["verified_literal_bytes"],
                               relocations=matched["relocations"],exact=matched["exact"])
                except ValueError as exc:
                    row["unverified"]=str(exc)
            print(json.dumps(row,sort_keys=True),flush=True)
            rows.append(row)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

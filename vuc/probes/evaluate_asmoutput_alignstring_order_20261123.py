#!/usr/bin/env python3
"""Test genuine AlignString predecessor around AsmOutput prologue.

Only the already exact AlignString source (no string literal rodata)
is joined to prologue. Strict literal authentication for prologue.
"""
import importlib.util,subprocess,json
from pathlib import Path
V=Path(__file__).resolve().parents[1]; O=Path("/tmp/vuc-asmoutput-alignstring-order-20261123");O.mkdir(exist_ok=True)
align=(V/"src/AlignString.cpp").read_text()
prologue=(V/"probes/asm_output_hypothesis.cpp").read_text()
prefix='#include <fstream>\n#include <cstring>\n'
align=align[align.index("char *AlignString("):]
prologue=prologue[prologue.index("void AsmOutput_WritePrologue("):]
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
flags={
 "o2":["-O2"],"rename":["-O2","-frename-registers"],
 "rename-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "rename-j4":["-O2","-frename-registers","-falign-jumps=4"],
 "rename-j8":["-O2","-frename-registers","-falign-jumps=8"],
 "j1":["-O2","-falign-jumps=1"],
 "j4":["-O2","-falign-jumps=4"],
 "j8":["-O2","-falign-jumps=8"]
}
arr={"PA":prologue+"\n"+align,"AP":align+"\n"+prologue,"P":prologue,
 "PAG":prologue+"\n"+align+"\nint constantsTabStop;",
 "GAP":"int constantsTabStop;\n"+align+"\n"+prologue,
 "GPA":"int constantsTabStop;\n"+prologue+"\n"+align,
 "APG":align+"\n"+prologue+"\nint constantsTabStop;"}
rows=[]
for name,body in arr.items():
 path=O/(name+".cpp");path.write_text(prefix+body+"\n")
 for tag,opts in flags.items():
  for compiler in ("jan25","dec06"):
   obj=O/(name+"-"+tag+"-"+compiler+".o")
   cmd=["/tmp/vuc-binutils215/g++-"+compiler+"-as215",*opts,"-c",str(path),"-o",str(obj)]
   p=subprocess.run(cmd,capture_output=True,text=True)
   r={"owner":name,"flags":tag,"compiler":compiler}
   if p.returncode:r["error"]=p.stderr[-200:]
   else:
    for k,c in (("prologue","asm_output_jan25_probe.json"),
                ("align","align_string_exact_oct13.json")):
     try:
      score=m.run(obj,V/"config"/c)
      r[k]={"match":score["exact_bytes"],"length":score["length"],"exact":score["exact"]}
     except ValueError as exc:r[k]={"unverified":str(exc)}
   rows.append(r);print(json.dumps(r,sort_keys=True),flush=True)
   if r.get("prologue",{}).get("exact"):
    (O/"results.json").write_text(json.dumps(rows,indent=2)+"\n");raise SystemExit(0)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

#!/usr/bin/env python3
"""Recover VUC AsmOutput prologue via genuine neighboring function emission order.

VCL's successful method-order technique is applied to the reconstructed
AsmOutput compilation unit, with real WriteEpilogue, AlignString,
AlignConstant and TabConstants definitions (no dummy calls or register
constraints). All original relocations and 92 literal bytes are checked.
Any exact sibling is separately scored to avoid regressions.
"""
import itertools,importlib.util,json,subprocess
from pathlib import Path
V=Path(__file__).resolve().parents[1]
OUT=Path("/tmp/vuc-asmoutput-definition-order-20261123");OUT.mkdir(exist_ok=True)
src=(V/"src/AsmOutputUnit.cpp").read_text()
src=src.replace('#include "MLGameString.h"',f'#include "{V/"src/MLGameString.h"}"')
begins=[
"void AsmOutput_WritePrologue(",
"void AsmOutput_WriteEpilogue(",
"char *AlignString(",
"char *AlignConstant(",
"void AsmOutput_TabConstants(",
]
positions=[src.index(x) for x in begins]
# Existing types/headers/externs need to precede the reordered definitions.
pre=src[:positions[0]]
pre+='\n#include <cstring>\n#include <vector>\n'
pre+='''extern int constantsTabStop;
class Instruction {
public:
    MLGameString text;
    int line;
    int kind;
    const char *ToString();
};
class InstructionList {
public:
    std::vector<Instruction *> instructions_;
};
'''
blocks={}
for i,key in enumerate("PEACT"):
    lo=positions[i]
    hi=positions[i+1] if i+1<len(positions) else len(src)
    section=src[lo:hi]
    if key=="E":section=section[:section.index("#include <cstring>")]
    if key=="A":section=section[:section.index("extern int constantsTabStop;")]
    if key=="C":section=section[:section.index("#include <cstring>")]
    if key=="T":section=section[section.index("void AsmOutput_TabConstants("):]
    blocks[key]=section
assert len(blocks)==5
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
configs={
 "prologue":"asm_output_jan25_probe.json",
 "epilogue":"asm_output_epilogue_probe.json",
 "tabs":"asm_output_tab_constants_probe.json"
}
sets={
 "baseline":["-O2"],
 "rename":["-O2","-frename-registers"],
 "rename-j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j1":["-O2","-falign-jumps=1"],
}
orders={}
# Preserve likely adjacent original owner, vary only prologue point of
# insertion and a handful of whole-TU semantic reorderings.
for group in ("EACT","EATC","AEC T".replace(" ",""),"TEAC","TACE","ETAC","CAET","CEAT","ACTE","ACET","CTEA","CETA"):
 for pos in range(5):
    seq=group[:pos]+"P"+group[pos:]
    orders[seq]=seq
rows=[]
for order in orders:
  path=OUT/(order+".cpp")
  path.write_text(pre+"\n".join(blocks[k] for k in order))
  for flags,opts in sets.items():
   for compiler in ("jan25","dec06"):
    obj=OUT/(order+"-"+flags+"-"+compiler+".o")
    proc=subprocess.run(["/tmp/vuc-binutils215/g++-"+compiler+"-as215",*opts,
                         "-c",str(path),"-o",str(obj)],capture_output=True,text=True)
    row={"order":order,"flags":flags,"compiler":compiler}
    if proc.returncode:row["compile_error"]=proc.stderr[-280:]
    else:
      try:
       x=m.run(obj,V/"config"/configs["prologue"])
       row["prologue"]={"size":x["length"],"match":x["exact_bytes"],
                        "exact":x["exact"],"diffs":x["different_offsets"]}
       if x["exact_bytes"]>22:
        for sibling in ("epilogue","tabs"):
         try:
          y=m.run(obj,V/"config"/configs[sibling])
          row[sibling]=[y["exact_bytes"],y["length"],y["exact"]]
         except ValueError as ex:row[sibling]={"unverified":str(ex)}
      except ValueError as ex:row["unverified"]=str(ex)
    rows.append(row)
    print(json.dumps(row,sort_keys=True),flush=True)
    if row.get("prologue",{}).get("exact",False):
     (OUT/"candidate.json").write_text(json.dumps(row,indent=2)+"\n")
     (OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
     raise SystemExit(0)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

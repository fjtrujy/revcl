#!/usr/bin/env python3
"""Authentic C++ class/owner context for VUC's one-byte-away arena cleanup.

GCC's deferred class implementation/header ownership can influence the
peephole2 temporary register even with an unchanged genuine destructor.
Test ordinary C++03 class declarations, original methods, and linkage; no
dummy functions or patched instructions.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/instruction-owner-variants-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
unit=(R/"vuc/probes/instruction_list_arena_unit_oct14.cpp").read_text().replace(
    '"../src/MLGameString.h"','"'+str(R/"vuc/src/MLGameString.h")+'"')
offset=unit.index("void InstructionList::Add(")
prefix=unit[:offset]
tail=unit[offset:]
options={
 "baseline":unit,
 "fn-sections":unit,
 "no-weak":unit,
 "split":'#include "InstructionOwnerNormal.h"\n'+tail,
 "deferred":'#pragma implementation "InstructionOwnerDeferred.h"\n#include "InstructionOwnerDeferred.h"\n'+tail,
 "normal-prag-implementation":'#pragma implementation "InstructionOwnerNormal.h"\n#include "InstructionOwnerNormal.h"\n'+tail,
 "interface-only":'#include "InstructionOwnerDeferred.h"\n'+tail,
}
(T/"InstructionOwnerNormal.h").write_text(prefix)
(T/"InstructionOwnerDeferred.h").write_text(prefix.replace("struct Instruction {","#pragma interface\nstruct Instruction {",1))
flags={
 "normal":["-O2","-frename-registers","-fuse-cxa-atexit"],
 "no-rename":["-O2","-fuse-cxa-atexit"],
 "jumpless":["-O2","-frename-registers","-fuse-cxa-atexit","-falign-jumps=1"],
}
results=[]
for name,text in options.items():
 p=T/(name+".cpp");p.write_text(text)
 for tag,base in flags.items():
  mods=base+(['-ffunction-sections'] if name=="fn-sections" else [])+(['-fno-weak'] if name=="no-weak" else [])
  for compiler_date,compiler in (("jan25","/tmp/vuc-binutils215/g++-jan25-as215"),("dec06","/tmp/vuc-binutils215/g++-dec06-as215")):
   obj=T/(name+"-"+tag+"-"+compiler_date+".o")
   cc=subprocess.run([compiler,*mods,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
   if cc.returncode:
    print("COMPILE",name,tag,compiler_date,cc.stderr[-150:],flush=True);continue
   try:
    r=m.run(obj,R/"vuc/config/instruction_arena_cleanup_unit_oct14_probe.json")
    results.append((r["exact_bytes"],r["length"],name,tag,compiler_date,r["different_offsets"][:8]))
    if r["exact"]:print("NEW_EXACT",p,mods,compiler_date,flush=True);raise SystemExit(0)
   except ValueError as e:
    results.append((0,0,name,tag,compiler_date,str(e)[:80]))
 print("DONE",name,flush=True)
print("BEST")
for row in sorted(results,reverse=True)[:30]:print(row)

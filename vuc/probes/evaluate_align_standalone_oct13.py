#!/usr/bin/env python3
"""Authenticate an isolated original-sized VUC AlignString source fragment.

Main's compiler-state source-order discovery produced a 145/145 function.
Verify that a minimal genuine C++03 function without adjacent helpers emits
the same exact x86 instructions, PLT calls and original static BSS reference.
"""
from pathlib import Path
import json,subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/align-minimal-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
unit=(R/"vuc/src/AsmOutputUnit.cpp").read_text()
fragment=unit[unit.index("char *AlignString("):unit.index("extern int constantsTabStop;")]
manifest=json.loads((R/"vuc/config/align_string_probe.json").read_text())
manifest.pop("relocation_literals",None)
manifest["source"]="vuc/src/AlignString.cpp"
config=T/"align-original-text-bss.json"
config.write_text(json.dumps(manifest,indent=2)+"\n")
cases={
"minimal":'#include <cstring>\n'+fragment,
"headers":'#include <fstream>\n#include <cstring>\n#include <vector>\n'+fragment,
"iostream":'#include <iostream>\n#include <cstring>\n'+fragment,
}
rank=[]
for label,source in cases.items():
 p=T/(label+".cpp");p.write_text(source)
 for compiler_name,compiler in (("jan25","/tmp/vuc-binutils215/g++-jan25-as215"),("dec06","/tmp/vuc-binutils215/g++-dec06-as215")):
  for flag_name,flags in (
   ("rj4",["-O2","-frename-registers","-falign-jumps=4"]),
   ("rj1",["-O2","-frename-registers","-falign-jumps=1"]),
   ("plain",["-O2","-falign-jumps=4"])
  ):
   o=T/(label+"-"+compiler_name+"-"+flag_name+".o")
   c=subprocess.run([compiler,*flags,"-c",str(p),"-o",str(o)],capture_output=True,text=True)
   if c.returncode:print("COMPILE_FAILED",label,compiler_name,flag_name,c.stderr[-90:],flush=True);continue
   row=next((s for s in subprocess.check_output(["nm","-S",str(o)],text=True).splitlines() if s.endswith("_Z11AlignStringPKci")),"")
   length=int(row.split()[1],16) if row else -1
   if length!=145:rank.append((0,length,label,compiler_name,flag_name));continue
   try:
    score=m.run(o,config)
    rank.append((score["exact_bytes"],length,label,compiler_name,flag_name))
    if score["exact"]:print("NEW_EXACT",p,compiler_name,flag_name,flush=True)
   except ValueError as e:print("REJECT",label,compiler_name,flag_name,str(e)[:95],flush=True)
print("BEST")
for row in sorted(rank,reverse=True)[:27]:print(row)

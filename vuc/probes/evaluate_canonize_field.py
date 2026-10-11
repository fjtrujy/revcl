#!/usr/bin/env python3
"""Explore original module guard's 32-bit type and load order."""
from pathlib import Path
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(R/"vuc/probes/canonize2_reduce.cpp").read_text()
fields={
 "int":"int flag;",
 "unsigned":"unsigned flag;",
 "volatile-int":"volatile int flag;",
 "volatile-unsigned":"volatile unsigned flag;",
 "const":"const int flag;",
 "enum":"enum Flag { Disabled=0, Enabled=1 }; Flag flag;",
 "bit32":"signed flag:32;",
 "bit32u":"unsigned flag:32;",
 "int-attr":"int flag __attribute__((aligned(4)));",
 "int-noalias":"int flag __attribute__((__may_alias__));",
}
scores=[]
for name,definition in fields.items():
 cpp=T/("canonize-field-"+name+".cpp");cpp.write_text(source.replace("int flag;",definition))
 obj=T/("canonize-field-"+name+".o")
 cp=subprocess.run([str(T/"g++-jan25-as215"),"-O2","-frename-registers","-falign-jumps=1","-I",str(R/"vuc/probes"),"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:scores.append((0,name,cp.stderr[-150:]));continue
 try:
  r=m.run(obj,R/"vuc/config/canonize2_reduce_probe.json")
  scores.append((r["exact_bytes"],name,str(r["length"])))
 except Exception as e:scores.append((0,name,str(e)[:85]))
scores.sort(reverse=True)
for row in scores:print(*row)

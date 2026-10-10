#!/usr/bin/env python3
"""Test ordinary i386-width member types for the VUC singleton guard.

Only a 106/106 comparison against original named BSS relocations can promote a
candidate; none of the generated source variants is accepted automatically.
"""
import importlib.util
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
tmp=Path("/tmp/vuc-binutils215")
spec=importlib.util.spec_from_file_location("matcher",root/"vuc/scripts/match_probe.py")
matcher=importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
base=(root/"vuc/probes/ml_string_table_get.cpp").read_text()
assert "    unsigned count_;\n    unsigned used_;" in base
assert "MLGameStringTable() : count_(0), used_(0) {}" in base
types={"vu":"volatile unsigned","vi":"volatile int","vl":"volatile long",
       "vul":"volatile unsigned long","u":"unsigned","i":"int","l":"long"}
modes={"init":"MLGameStringTable() : count_(0), used_(0) {}",
       "body":"MLGameStringTable() { count_=0; used_=0; }",
       "reverse":"MLGameStringTable() { used_=0; count_=0; }"}
result=[]
for left in ("vu","vi","vl","vul","u","i","l"):
 for right in ("vu","vi","vl","vul","u","i","l"):
  for mode in modes:
   key=left+"-"+right+"-"+mode
   source=base.replace("    unsigned count_;\n    unsigned used_;",
       "    "+types[left]+" count_;\n    "+types[right]+" used_;")
   source=source.replace("MLGameStringTable() : count_(0), used_(0) {}",modes[mode])
   dest=tmp/("get-types-"+key+".cpp")
   dest.write_text(source)
   obj=tmp/("get-types-"+key+".o")
   run=subprocess.run([str(tmp/"g++-jan25-as215"),"-O2",
     "-frename-registers","-fuse-cxa-atexit","-c",str(dest),"-o",str(obj)],
     capture_output=True,text=True)
   if run.returncode:
    result.append((0,key,"compiler failure"));continue
   try:
    d=matcher.run(obj,root/"vuc/config/ml_string_table_get_probe.json")
    if d["exact"]:
     print("EXACT",key,flush=True)
     raise SystemExit(0)
    result.append((d["exact_bytes"],key,str(d["length"])))
   except Exception as exc:result.append((0,key,str(exc)[:65]))
  print("tested",left,right,flush=True)
result.sort(reverse=True)
print("best outcomes:",*result[:24],sep="\n")

#!/usr/bin/env python3
"""Cross-check the two original VUC GCC 3.3.3 build dates for near-exact bodies.

The original ELF embeds both 20031206 and 20040125 compiler strings.
Do not assume they identify the source unit for any function. Verify
full relocations and length with existing accepted experimental manifests.
"""
from pathlib import Path
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/near-exact-compiler-dates-oct13");T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
sources={
 "align":("vuc/src/AsmOutputUnit.cpp","vuc/config/align_string_probe.json"),
 "arena":("vuc/probes/instruction_list_arena_unit_oct14.cpp","vuc/config/instruction_arena_cleanup_unit_oct14_probe.json")
}
flags={
 "original":["-O2","-frename-registers","-falign-jumps=4"],
 "cxa":["-O2","-frename-registers","-fuse-cxa-atexit"],
 "cxa-sections":["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"],
 "plain":["-O2"],
 "reg":["-O2","-frename-registers"],
}
for target,(name,cfg) in sources.items():
 for date,compiler in {
     "dec06":"/tmp/vuc-binutils215/g++-dec06-as215",
     "jan25":"/tmp/vuc-binutils215/g++-jan25-as215"}.items():
  for label,opts in flags.items():
   obj=T/(target+"-"+date+"-"+label+".o")
   cp=subprocess.run([compiler,*opts,"-c",str(R/name),"-o",str(obj)],capture_output=True,text=True)
   if cp.returncode:print("ERROR",target,date,label,cp.stderr[-125:],flush=True);continue
   try:
    result=m.run(obj,R/cfg)
    print("MATCH",target,date,label,result["exact_bytes"],result["length"],result["different_offsets"][:8],flush=True)
    if result["exact"]:print("NEW_EXACT",target,date,label,flush=True)
   except ValueError as e:print("REJECT",target,date,label,str(e)[:110],flush=True)

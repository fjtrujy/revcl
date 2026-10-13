#!/usr/bin/env python3
"""Historically plausible GCC flags for real Value TypeConvert/Force unit.

The 257-byte TypeConvertForce partial includes real TypeConvert as its
source predecessor. Probe optimizer decisions without inline assembler
or source-only register/padding constructs.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/typeconvertforce-compiler-flags-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=R/"vuc/probes/ValueTypeConvertForceUnit.cpp"
cfg=R/"vuc/config/value_type_convert_force_oct13_probe.json"
changes={
 "base":[],
 "no-gcse":["-fno-gcse"],
 "no-regmove":["-fno-regmove"],
 "no-ifconv":["-fno-if-conversion"],
 "no-ifconv2":["-fno-if-conversion2"],
 "no-crossjump":["-fno-crossjumping"],
 "no-sched2":["-fno-schedule-insns2"],
 "no-reorder":["-fno-reorder-blocks"],
 "no-guess":["-fno-guess-branch-probability"],
 "no-caller-saves":["-fno-caller-saves"],
 "no-peephole":["-fno-peephole"],
 "no-peephole2":["-fno-peephole2"],
 "no-cse-follow":["-fno-cse-follow-jumps"],
 "no-deferpop":["-fno-defer-pop"],
 "no-strength":["-fno-strength-reduce"],
 "no-expensive":["-fno-expensive-optimizations"],
 "no-builtin":["-fno-builtin"],
 "no-fun-cse":["-fno-function-cse"],
 "no-inline":["-fno-inline"],
 "no-strictalias":["-fno-strict-aliasing"],
 "param-inline":["--param","max-inline-insns-single=1"],
 "fomit":["-fomit-frame-pointer"],
 "fnoomit":["-fno-omit-frame-pointer"],
 "arch386":["-march=i386"],
 "arch486":["-march=i486"],
 "arch686":["-march=i686"],
 "stack4":["-mpreferred-stack-boundary=2"],
}
rank=[]
for name,opts in changes.items():
 for date,compiler in (("jan25","/tmp/vuc-binutils215/g++-jan25-as215"),("dec06","/tmp/vuc-binutils215/g++-dec06-as215")):
  for rename in (False,True):
   flags=["-O2"]+(["-frename-registers"] if rename else [])+["-falign-jumps=1","-falign-labels=1","-falign-loops=2",*opts]
   label=name+"-"+date+("-ren" if rename else "-plain")
   obj=T/(label+".o")
   cp=subprocess.run([compiler,*flags,"-c",str(source),"-o",str(obj)],capture_output=True,text=True)
   if cp.returncode:
    print("COMPILE",label,cp.stderr[-95:],flush=True);continue
   output=subprocess.check_output(["nm","-S",str(obj)],text=True)
   entry=next((line for line in output.splitlines() if line.endswith("_ZN5Value16TypeConvertForceE6TypeIDRKS_S2_RS_S3_")),"")
   size=int(entry.split()[1],16) if entry else -1
   if size!=257:
    rank.append((0,label,size));continue
   try:
    result=m.run(obj,cfg)
    rank.append((result["exact_bytes"],label,size))
    if result["exact"]:
     print("NEW_EXACT",label,str(source),flush=True);raise SystemExit(0)
   except ValueError as e:rank.append((0,label,str(e)[:50]))
  print("CHECKED",name,date,flush=True)
print("TOP")
for row in sorted(rank,reverse=True)[:38]:print(row)

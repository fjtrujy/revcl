#!/usr/bin/env python3
"""Find historical GCC flags for VUC MLTempString::Set with corrected return ABI."""
import importlib.util,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
flags={
 "o2":[],
 "rename":["-frename-registers"],
 "nojump":["-falign-jumps=1"],
 "rename-nojump":["-frename-registers","-falign-jumps=1"],
 "no-sched2":["-fno-schedule-insns2"],
 "rename-no-sched2":["-frename-registers","-fno-schedule-insns2"],
 "no-gcse":["-fno-gcse"],
 "rename-no-gcse":["-frename-registers","-fno-gcse"],
 "no-regmove":["-fno-regmove"],
 "rename-no-regmove":["-frename-registers","-fno-regmove"],
 "no-cse":["-fno-cse-follow-jumps"],
 "rename-no-cse":["-frename-registers","-fno-cse-follow-jumps"],
 "no-strict":["-fno-strict-aliasing"],
 "rename-no-strict":["-frename-registers","-fno-strict-aliasing"],
 "no-peephole":["-fno-peephole2"],
 "rename-no-peephole":["-frename-registers","-fno-peephole2"],
 "no-ifconv":["-fno-if-conversion"],
 "rename-no-ifconv":["-frename-registers","-fno-if-conversion"],
 "no-reorder":["-fno-reorder-blocks"],
 "rename-no-reorder":["-frename-registers","-fno-reorder-blocks"],
 "align-labels1":["-falign-labels=1"],
 "rename-align-labels1":["-frename-registers","-falign-labels=1"],
 "align-loops1":["-falign-loops=1"],
 "rename-align-loops1":["-frename-registers","-falign-loops=1"],
 "march-i386":["-march=i386"],
 "march-i686":["-march=i686"],
 "rename-march-i686":["-frename-registers","-march=i686"],
 "no-omit":["-fno-omit-frame-pointer"],
 "rename-no-omit":["-frename-registers","-fno-omit-frame-pointer"],
}
scores=[]
for name,extra in flags.items():
 obj=T/("temp-return-flags-"+name+".o")
 cp=subprocess.run([str(T/"g++-jan25-as215"),"-O2",*extra,"-c",str(R/"vuc/probes/ml_temp_set_return.cpp"),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode: scores.append((0,name,cp.stderr[-90:]));continue
 try:
  result=m.run(obj,R/"vuc/config/ml_temp_set_probe.json")
  scores.append((result["exact_bytes"],name,str(result["length"])))
 except Exception as e:scores.append((0,name,str(e)[:85]))
scores.sort(reverse=True)
for rec in scores:print(*rec)

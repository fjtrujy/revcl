#!/usr/bin/env python3
"""Historical GCC flag probes for Print(ASTFunCall*), retaining strict acceptance."""
from pathlib import Path
import importlib.util, subprocess
ROOT=Path(__file__).resolve().parents[2]
TMP=Path("/tmp/vuc-binutils215")
spec=importlib.util.spec_from_file_location("vucmatcher",ROOT/"vuc/scripts/match_probe.py")
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
flags={
 "plain":[],
 "no-gcse":["-fno-gcse"],
 "no-sched2":["-fno-schedule-insns2"],
 "no-regmove":["-fno-regmove"],
 "no-cse":["-fno-cse-follow-jumps"],
 "no-strict":["-fno-strict-aliasing"],
 "no-reorder":["-fno-reorder-blocks"],
 "no-crossjump":["-fno-crossjumping"],
 "no-peephole":["-fno-peephole2"],
 "no-ifconv":["-fno-if-conversion"],
 "no-omit":["-fno-omit-frame-pointer"],
 "no-tree":["-fno-expensive-optimizations"],
 "align-j1":["-falign-jumps=1"],
 "align-labels1":["-falign-labels=1"],
 "align-loops1":["-falign-loops=1"],
 "align-functions1":["-falign-functions=1"],
 "march-i386":["-march=i386"],
 "march-i486":["-march=i486"],
 "march-i586":["-march=i586"],
 "march-i686":["-march=i686"],
 "tune-i486":["-mcpu=i486"],
 "tune-i586":["-mcpu=i586"],
 "tune-i686":["-mcpu=i686"],
 "j1-label1":["-falign-jumps=1","-falign-labels=1"],
 "no-regmove-nosched":["-fno-regmove","-fno-schedule-insns2"],
 "no-peephole-nosched":["-fno-peephole2","-fno-schedule-insns2"],
}
scores=[]
for name,extra in flags.items():
 obj=TMP/("funprint-flags-"+name+".o")
 cp=subprocess.run([str(TMP/"g++-jan25-as215"),"-O2",*extra,"-c",str(ROOT/"vuc/probes/print_ast_fun_call.cpp"),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:scores.append((0,name,"compile:"+cp.stderr[-90:]));continue
 try:
  m=matcher.run(obj,ROOT/"vuc/config/print_ast_fun_call_probe.json")
  scores.append((m["exact_bytes"],name,str(m["length"])))
 except Exception as e:scores.append((0,name,str(e)[:95]))
scores.sort(reverse=True)
for item in scores:print(*item)

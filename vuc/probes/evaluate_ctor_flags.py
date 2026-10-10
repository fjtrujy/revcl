#!/usr/bin/env python3
"""Source-date GCC flag sweep of both VUC varargs constructor ABI entries."""
from pathlib import Path
import importlib.util, subprocess
ROOT=Path(__file__).resolve().parents[2]
TOOL=Path("/tmp/vuc-binutils215")
spec=importlib.util.spec_from_file_location("vuc_match",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cc=str(TOOL/"g++-jan25-as215")
src=ROOT/"vuc/probes/ml_temp_format_inline.cpp"
c2=ROOT/"vuc/config/ml_temp_format_c2_probe.json"
c1=ROOT/"vuc/config/ml_temp_format_c1_probe.json"
flags=[
["-O2","-frename-registers","-falign-jumps=1"],
["-O2","-frename-registers","-falign-jumps=1","-falign-labels=1"],
["-O2","-frename-registers","-falign-jumps=1","-falign-loops=1"],
["-O2","-frename-registers","-falign-jumps=1","-falign-functions=1"],
["-O2","-frename-registers","-falign-jumps=1","-fno-reorder-blocks"],
["-O2","-frename-registers","-falign-jumps=1","-fno-schedule-insns2"],
["-O2","-frename-registers","-falign-jumps=1","-fno-gcse"],
["-O2","-frename-registers","-falign-jumps=1","-fno-peephole2"],
["-O2","-frename-registers","-falign-jumps=1","-fno-cse-follow-jumps"],
["-O2","-frename-registers","-falign-jumps=1","-fno-optimize-sibling-calls"],
["-O2","-frename-registers","-falign-jumps=1","-fno-strict-aliasing"],
["-O2","-frename-registers","-falign-jumps=1","-fomit-frame-pointer"],
["-O2","-frename-registers","-falign-jumps=1","-fno-omit-frame-pointer"],
["-O2","-frename-registers","-falign-jumps=1","-march=i386"],
["-O2","-frename-registers","-falign-jumps=1","-march=i486"],
["-O2","-frename-registers","-falign-jumps=1","-march=i586"],
["-O2","-frename-registers","-falign-jumps=1","-march=i686"],
["-O2","-frename-registers","-falign-jumps=1","-mcpu=i386"],
["-O2","-frename-registers","-falign-jumps=1","-mcpu=i486"],
["-O2","-frename-registers","-falign-jumps=1","-mcpu=i586"],
["-O2","-frename-registers","-falign-jumps=1","-mcpu=i686"],
["-O2","-frename-registers","-falign-jumps=1","-fno-if-conversion"],
["-O2","-frename-registers","-falign-jumps=1","-fno-crossjumping"],
["-O2","-frename-registers","-falign-jumps=1","-fno-regmove"],
]
out=[]
for i,opts in enumerate(flags):
 obj=TOOL/("format-flags-%02d.o"%i)
 cp=subprocess.run([cc,*opts,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:
  out.append((0,0,"FAIL",i," ".join(opts),cp.stderr[-90:]));continue
 scores=[]
 for cfg in (c2,c1):
  try:
   r=m.run(obj,cfg)
   scores.append((r["exact_bytes"],r["length"]))
  except Exception as ex:scores.append((0,str(ex)[:45]))
 out.append((scores[0][0],scores[1][0],str(scores),i," ".join(opts)))
out.sort(reverse=True)
for r in out:print(*r)

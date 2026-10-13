#!/usr/bin/env python3
"""Bounded VUC GCC3.3 optimizer pass controls for genuine Flex next-buffer.

Any score requires original length, all four authenticated relocations and
152 original-mapped initialized bytes. Different optimizer behavior is
diagnostic only; no simulated machine-code patch or fake predecessor.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/yy-next-opt-controls");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=R/'vuc/probes/yy_get_next_buffer_partial_oct13.cpp'
conf=R/'vuc/config/yy_get_next_buffer_oct13_probe.json'
changes={
 "baseline":[],
 "no-gcse":["-fno-gcse"],
 "no-regmove":["-fno-regmove"],
 "no-if-conversion":["-fno-if-conversion"],
 "no-if-conversion2":["-fno-if-conversion2"],
 "no-schedule2":["-fno-schedule-insns2"],
 "no-cse-follow":["-fno-cse-follow-jumps"],
 "no-peephole2":["-fno-peephole2"],
 "no-reorder-blocks":["-fno-reorder-blocks"],
 "no-crossjump":["-fno-crossjumping"],
 "no-guess":["-fno-guess-branch-probability"],
 "no-expensive":["-fno-expensive-optimizations"],
 "no-caller-saves":["-fno-caller-saves"],
 "no-strict-aliasing":["-fno-strict-aliasing"],
 "no-builtin":["-fno-builtin"],
 "no-inline":["-fno-inline"],
 "plain-ren-j2":["-falign-jumps=2"],
}
results=[]
for key,extras in changes.items():
 for compiler_name,compiler in [('jan25','/tmp/vuc-binutils215/g++-jan25-as215'),('dec06','/tmp/vuc-binutils215/g++-dec06-as215')]:
  opts=['-O2','-frename-registers','-falign-jumps=1',*extras]
  obj=T/(key+'-'+compiler_name+'.o')
  p=subprocess.run([compiler,*opts,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  if p.returncode:print('ERROR',key,compiler_name,p.stderr[-95:],flush=True);continue
  row=next((x for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if x.endswith('_ZN11yyFlexLexer18yy_get_next_bufferEv')),"")
  size=int(row.split()[1],16) if row else -1
  if size!=550:results.append((0,size,key,compiler_name));continue
  try:
   score=m.run(obj,conf)
   results.append((score['exact_bytes'],size,key,compiler_name))
   if score['exact']:
    print('NEW_EXACT',key,compiler_name,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',key,compiler_name,str(e)[:110],flush=True)
 print('DONE',key,flush=True)
print('BEST')
for row in sorted(results,reverse=True)[:27]:print(row)

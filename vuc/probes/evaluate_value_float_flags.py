#!/usr/bin/env python3
"""Historical GCC 3.3 flags for original VUC floating Value comparison code."""
from pathlib import Path
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/valuecomp-sweep');T.mkdir(exist_ok=True)
sp=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
mod=importlib.util.module_from_spec(sp);sp.loader.exec_module(mod)
c='/tmp/vuc-binutils215/g++-jan25-as215'
options=[
[],
['-ffast-math'],['-funsafe-math-optimizations'],['-fno-trapping-math'],
['-ffinite-math-only'],['-fno-math-errno'],['-fno-signaling-nans'],
['-fno-rounding-math'],['-fno-signed-zeros'],['-ffloat-store'],
['-fno-strict-aliasing'],['-fno-delete-null-pointer-checks'],
['-fno-gcse'],['-fno-regmove'],['-fno-schedule-insns2'],
['-fno-cse-follow-jumps'],['-fno-peephole2'],['-fno-reorder-blocks'],
['-fno-omit-frame-pointer'],['-march=i686'],['-mtune=i686'],
['-ffast-math','-fno-schedule-insns2'],['-ffast-math','-fno-gcse'],
['-ffast-math','-fno-regmove'],['-ffast-math','-ffloat-store'],
['-funsafe-math-optimizations','-ffinite-math-only'],
]
for i,ext in enumerate(options):
 flag=['-O2','-frename-registers','-falign-jumps=1',*ext]
 obj=T/f'comp-{i:02}.o'
 proc=subprocess.run([c,*flag,'-c',str(R/'vuc/probes/value_comparison.cpp'),'-o',str(obj)],capture_output=True,text=True)
 results=[]
 if proc.returncode:
  print(i,*ext,'compile failed',proc.stderr[-130:],flush=True);continue
 for case in ['eq','ne']:
  cfg=R/f'vuc/config/value_comparison_{case}_probe.json'
  try:
   r=mod.run(obj,cfg);results.append(f'{case} {r["exact_bytes"]}/{r["length"]}' + (' EXACT' if r['exact'] else ''))
  except Exception as e:results.append(f'{case} unavailable: {str(e)[:50]}')
 print(i,' '.join(ext) if ext else 'baseline','; '.join(results),flush=True)

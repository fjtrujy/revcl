#!/usr/bin/env python3
"""Explore period-appropriate GCC 3.3 flags for Value multiply register choice."""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2];T=Path('/tmp/vuc-binutils215')
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
src=R/'vuc/probes/value_ml.cpp'
extra=['-fno-regmove','-fno-cse-follow-jumps','-fno-cse-skip-blocks','-fno-if-conversion',
       '-fno-if-conversion2','-fno-peephole2','-fno-reorder-blocks','-fno-strict-aliasing',
       '-fno-inline','-fno-unit-at-a-time','-fno-strength-reduce','-fno-rerun-cse-after-loop',
       '-fno-web','-fno-expensive-optimizations','-fno-optimize-sibling-calls',
       '-fno-delayed-branch','-fno-guess-branch-probability','-fno-branch-count-reg',
       '-march=i386','-march=i486','-march=i586','-march=i686','-mtune=i386',
       '-mtune=i486','-mtune=i686','-fno-builtin','-fno-omit-frame-pointer',
       '-falign-loops=1','-falign-labels=1','-falign-functions=1','-falign-jumps=2',
       '-falign-jumps=3','-falign-jumps=4']
results=[]
for n,opt in enumerate(['']+extra):
 args=['-O2','-frename-registers','-falign-jumps=1']+([opt] if opt else [])
 obj=T/('vmul-flags-%02d.o'%n)
 cp=subprocess.run([str(T/'g++-jan25-as215'),*args,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:results.append((0,opt,cp.stderr[-85:]));continue
 try:
  d=m.run(obj,R/'vuc/config/value_ml_probe.json');results.append((d['exact_bytes'],opt,str(d['length'])))
 except Exception as e:results.append((0,opt,str(e)[:85]))
 if results[-1][0]==222:print('EXACT',opt,flush=True);break
print('BEST FLAGS:')
for item in sorted(results,reverse=True)[:25]:print(*item)

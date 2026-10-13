#!/usr/bin/env python3
"""Bounded historical GCC 3.3.3 register-scheduling controls for AlignString.

Only the authentic AsmOutput source unit and period-appropriate compiler
options are tested. Neither original binary nor emitted code is patched.
"""
import importlib.util,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/align-space-flags-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
extras=['','-fno-regmove','-fno-web','-fno-gcse','-fno-cse-follow-jumps',
 '-fno-cse-skip-blocks','-fno-peephole2','-fno-if-conversion',
 '-fno-if-conversion2','-fno-schedule-insns2','-fno-schedule-insns',
 '-fno-strict-aliasing','-fno-unit-at-a-time','-fno-reorder-blocks',
 '-fno-expensive-optimizations','-fno-rerun-cse-after-loop',
 '-fno-guess-branch-probability','-fno-optimize-sibling-calls',
 '-fno-inline','-fno-builtin','-falign-jumps=1','-falign-jumps=8',
 '-falign-labels=1','-falign-labels=4','-falign-loops=1',
 '-falign-loops=4','-falign-functions=1','-falign-functions=4',
 '-march=i386','-march=i486','-march=i586','-march=i686',
 '-mtune=i386','-mtune=i486','-mtune=i686','-fmerge-all-constants']
for i,opt in enumerate(extras):
 flags=['-O2','-frename-registers','-falign-jumps=4']+([opt] if opt else [])
 obj=T/('flags-%02d.o'%i)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(R/'vuc/src/AsmOutputUnit.cpp'),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:
  print('COMPILE_FAIL',opt,cp.stderr[-100:],flush=True);continue
 try:
  a=m.run(obj,R/'vuc/config/align_string_probe.json')
  t=m.run(obj,R/'vuc/config/asm_output_tab_constants_probe.json')
  print('CHECK',opt or '<baseline>',f'align={a["exact_bytes"]}/{a["length"]}',f'tab={t["exact_bytes"]}/{t["length"]}',flush=True)
  if a['exact']:
   print('ALIGN BYTE EXACT',opt, 'Tab exact:',t['exact'],flush=True)
   break
 except ValueError as e:
  print('REJECT',opt,str(e)[:145],flush=True)

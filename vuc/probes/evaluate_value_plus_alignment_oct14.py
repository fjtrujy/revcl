#!/usr/bin/env python3
"""Recover original Value addition layout using authentic GCC alignments.

The original and natural compiled source agree until the GCC-generated
two-byte vector-arm padding: original vector entry +0xa7, candidate +0xa9.
Sweep real historical jump/label/loop alignment controls. Preserve the
actual +,-,* source-unit predecessor order and original TypeConvert relocation.
"""
from itertools import product
from pathlib import Path
import importlib.util,subprocess,json
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/value-plus-natural-alignment-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=json.loads((R/'vuc/config/value_arithmetic_plus_probe.json').read_text())
base['source']='vuc/src/ValueArithmeticUnit.cpp'
cfg=T/'plus-real-unit.json';cfg.write_text(json.dumps(base))
r=[]
for j,l,k in product((1,2,3,4,8),(1,2,4,8),(1,2,4,8,16)):
 opts=['-O2','-frename-registers',f'-falign-jumps={j}',f'-falign-labels={l}',f'-falign-loops={k}']
 label=f'j{j}-l{l}-loop{k}'
 o=T/(label+'.o')
 run=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(R/'vuc/src/ValueArithmeticUnit.cpp'),'-o',str(o)],capture_output=True,text=True)
 if run.returncode:
  print('ERROR',label,run.stderr[-150:],flush=True);continue
 nm=subprocess.check_output(['nm','-S',str(o)],text=True)
 record=next((x for x in nm.splitlines() if x.split()[-1:]==['_ZNK5ValueplERKS_']),'')
 sz=int(record.split()[1],16) if record else -1
 if sz==219:
  try:
   sc=m.run(o,cfg)
   r.append((sc['exact_bytes'],sz,label,sc['different_offsets'][:14]))
   if sc['exact']:
    print('NEW_EXACT',label,opts,flush=True)
    raise SystemExit(0)
  except ValueError as e:print('REJECT',label,str(e)[:130],flush=True)
 else:r.append((0,sz,label))
print('TOP')
for row in sorted(r,reverse=True)[:36]:print(row)

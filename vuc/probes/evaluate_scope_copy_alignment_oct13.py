#!/usr/bin/env python3
"""Search natural alignment only for original VUC 120-byte scope-copy template.

Use authentic historical std::__uninitialized_copy_aux instantiation;
do not construct instructions by hand or waive original relocations.
"""
from pathlib import Path
from itertools import product
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]; T=Path('/tmp/vuc-binutils215/scope-copy-try-oct13');T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
sources={'instantiation':R/'vuc/probes/hash_scope_uninitialized.cpp','cleanup':R/'vuc/probes/scope_uninitialized_cleanup_order.cpp'}
flags=[(j,l) for j,l in product((1,2,3,4,5,8),(1,2,4,8))]
scores=[]
for name,path in sources.items():
 for jump,label in flags:
  opts=['-O2','-frename-registers',f'-falign-jumps={jump}',f'-falign-labels={label}']
  obj=T/(name+f'-j{jump}-l{label}.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(path),'-o',str(obj)],capture_output=True,text=True)
  if p.returncode:print('ERROR',name,jump,label,p.stderr[-110:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  row=next((x for x in nm.splitlines() if x.endswith('_ZSt24__uninitialized_copy_auxIP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS1_ELi513EES7_ET0_T_S9_S8_12__false_type')),'')
  sz=int(row.split()[1],16) if row else -1
  if sz!=120:
   scores.append((0,sz,name,jump,label));continue
  try:
   r=m.run(obj,R/'vuc/config/hash_scope_uninitialized_probe.json')
   scores.append((r['exact_bytes'],sz,name,jump,label))
   if r['exact']:print('NEW_EXACT',name,jump,label);raise SystemExit(0)
  except ValueError as e:print('REJECT',name,jump,label,str(e)[:130],flush=True)
 print('DONE',name,flush=True)
print('TOP')
for row in sorted(scores,reverse=True)[:30]:print(row)

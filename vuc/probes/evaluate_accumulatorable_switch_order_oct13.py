#!/usr/bin/env python3
"""Test original VUC IsAccumulatorable switch lexical order with GCC 3.3.3.

Motivation: the newly rebased VCL Operand::Emit evidence shows that case
order affects peephole2/rnreg even when physical blocks look unchanged.
Use only meaningful conditional/switch dispatch preserving the true
11/13/12 accumulator classification, without artificial padding.
"""
from pathlib import Path
import itertools, importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accum-switch-order-oct13')
T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location('matcher',R/'vuc/scripts/match_probe.py')
matcher=importlib.util.module_from_spec(spec);spec.loader.exec_module(matcher)
header=(R/'vuc/probes/ir_accumulatorable.cpp').read_text()
header=header[:header.index('bool IsAccumulatorable')]
cond='op->operand->kind == 3'
patterns={}
for order in itertools.permutations((11,13,12)):
 name=''.join(map(str,order))
 patterns['if-'+name]='\n'.join('    if (op->kind == %d && %s) return true;' % (c,cond) for c in order)+'\n    return false;'
 patterns['ifnested-'+name]='\n'.join('    if (op->kind == %d) { if (%s) return true; }' % (c,cond) for c in order)+'\n    return false;'
 patterns['else-'+name]='\n'.join(('    if' if i==0 else '    else if')+' (op->kind == %d) { if (%s) return true; }' % (c,cond) for i,c in enumerate(order))+'\n    return false;'
 patterns['switch-break-'+name]='    switch(op->kind) {\n'+ '\n'.join('        case %d: if (%s) return true; break;' % (c,cond) for c in order)+'\n    }\n    return false;'
 patterns['switch-return-'+name]='    switch(op->kind) {\n'+ '\n'.join('        case %d: return %s;' % (c,cond) for c in order)+'\n    }\n    return false;'
 patterns['switch-flag-'+name]='    bool result = false;\n    switch(op->kind) {\n'+ '\n'.join('        case %d: if (%s) result = true; break;' % (c,cond) for c in order)+'\n    }\n    return result;'
 flags={
  'rn-j1':['-O2','-frename-registers','-falign-jumps=1'],
  'plain-j1':['-O2','-falign-jumps=1'],
  'rn-j4':['-O2','-frename-registers','-falign-jumps=4'],
  'rn-j8':['-O2','-frename-registers','-falign-jumps=8'],
 }
rank=[]
for name,body in patterns.items():
 src=T/(name+'.cpp');src.write_text(header+'\nbool IsAccumulatorable(IR::Operator_Data *op) {\n'+body+'\n}\n')
 for fl,opts in flags.items():
  obj=T/(name+'-'+fl+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode: print('COMPILE_FAIL',name,fl,cp.stderr[-135:],flush=True);continue
  lines=subprocess.run(['nm','-S',str(obj)],capture_output=True,text=True).stdout.splitlines()
  sym=next((x for x in lines if '_Z17IsAccumulatorable' in x),'')
  size=int(sym.split()[1],16) if sym else 0
  if size==77:
   try:
    result=matcher.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
    rank.append((result['exact_bytes'],name,fl,size))
    if result['exact']:
     print('BYTE-EXACT',name,fl,src,flush=True)
     raise SystemExit(0)
   except ValueError as exc:print('COMPARE_REJECT',name,fl,str(exc)[:105],flush=True)
  else:rank.append((0,name,fl,size))
 print('checked',name,flush=True)
for row in sorted(rank,reverse=True)[:26]:print('BEST',row)

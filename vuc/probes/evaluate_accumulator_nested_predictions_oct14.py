#!/usr/bin/env python3
"""Probe source-level boolean identities and old-GCC jump layout for VUC.

The previous 74/77 candidate differs solely in case-11 branch orientation.
Try nested prediction hints and original-era alignment knobs without changing
runtime truth values, creating fake helpers, or patching assembler bytes.
"""
from pathlib import Path
import subprocess,itertools,importlib.util,os,collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accum-nested-pred-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
original=(R/'vuc/probes/ir_accumulatorable_joint_oct14.cpp').read_text()
old='__builtin_expect(op->operand->kind == 3, 1)'
assert original.count(old)==1
expr='op->operand->kind == 3'
neg='op->operand->kind != 3'
forms={
'baseline':old,
'plain':expr,
'likely':'__builtin_expect('+expr+', 1)',
'unlikely':'__builtin_expect('+expr+', 0)',
'like-like':'__builtin_expect(__builtin_expect('+expr+',1),1)',
'like-unlike':'__builtin_expect(__builtin_expect('+expr+',1),0)',
'unlike-like':'__builtin_expect(__builtin_expect('+expr+',0),1)',
'unlike-unlike':'__builtin_expect(__builtin_expect('+expr+',0),0)',
'double-not':'!!__builtin_expect('+expr+',1)',
'neg-like':'!__builtin_expect('+neg+',0)',
'neg-unlike':'!__builtin_expect('+neg+',1)',
'nested-neg':'!__builtin_expect(__builtin_expect('+neg+',0),0)',
'nested-neg01':'!__builtin_expect(__builtin_expect('+neg+',0),1)',
'bool-static':'static_cast<bool>(__builtin_expect('+expr+',1))',
'logical-eq':'__builtin_expect(('+expr+') == true,1)',
'disjunct':'__builtin_expect('+expr+',1) || false',
'conjunct':'__builtin_expect('+expr+',1) && true',
'complement':'(__builtin_expect('+neg+',0) == false)',
'complement-not':'!(__builtin_expect('+neg+',0) != false)',
}
settings={
'j8':['-O2','-frename-registers','-falign-jumps=8'],
'j4-l8':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=8'],
'j8-l8':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=8'],
'j4-loop8':['-O2','-frename-registers','-falign-jumps=4','-falign-loops=8'],
'j8-loop8':['-O2','-frename-registers','-falign-jumps=8','-falign-loops=8'],
'j8-loop1':['-O2','-frename-registers','-falign-jumps=8','-falign-loops=1'],
'j8-l4':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=4'],
'j8-l2':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=2'],
'j8-noreorder':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
'j8-cost0':['-O2','-frename-registers','-falign-jumps=8','-mbranch-cost=0'],
'j8-no-guess':['-O2','-frename-registers','-falign-jumps=8','-fno-guess-branch-probability'],
}
compilers={'jan25':'/tmp/vuc-binutils215/g++-jan25-as215',
'dec06':'/tmp/vuc-binutils215/g++-dec06-as215'}
scores=[];sizes=collections.Counter()
for name,expression in forms.items():
 source=T/(name+'.cpp')
 source.write_text(original.replace(old,expression))
 for snap,compiler in compilers.items():
  for flag,opts in settings.items():
   obj=T/(name+'-'+snap+'-'+flag+'.o')
   cp=subprocess.run([compiler,*opts,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
   if cp.returncode:
    print('COMPILE_ERROR',name,snap,flag,cp.stderr[-100:],flush=True);continue
   sym=next((s for s in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
    if s.split()[-1:]==['_Z17IsAccumulatorablePN2IR13Operator_DataE']),'')
   length=int(sym.split()[1],16) if sym else -1;sizes[length]+=1
   if length!=77:continue
   try:
    result=m.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
    scores.append((result['exact_bytes'],name,snap,flag,result['different_offsets'][:12]))
    if result['exact']:
     print('NEW_EXACT',source,snap,flag,obj,flush=True);raise SystemExit(0)
   except ValueError as e: scores.append((0,name,snap,flag,str(e)[:85]))
  print('CHECKED',name,snap,flush=True)
print('LENGTHS',sizes.most_common(15))
print('TOP')
for x in sorted(scores,reverse=True)[:35]:print(x,flush=True)

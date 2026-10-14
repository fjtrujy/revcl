#!/usr/bin/env python3
"""VUC 77-byte IsAccumulatorable: joint GCC 3.3 prediction/alignment test.

The latest VCL reduceExtents required *two* simultaneously unobservable
prediction annotations to reproduce one otherwise unexplained alignment.
Test actual boolean predicates here, not injected assembly/NOPs.
"""
import itertools,subprocess,importlib.util,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
OUT=Path('/tmp/vuc-binutils215/accumulator-joint-probabilities-oct14')
OUT.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('matcher',ROOT/'vuc/scripts/match_probe.py')
matcher=importlib.util.module_from_spec(sp);sp.loader.exec_module(matcher)
header=(ROOT/'vuc/probes/ir_accumulatorable.cpp').read_text().split('bool IsAccumulatorable(',1)[0]
baseline='''bool IsAccumulatorable(IR::Operator_Data *op)
{
    if (op->kind == 11) {
        if (op->operand->kind == 3) return true;
    }
    if (op->kind == 13) {
        if (op->operand->kind == 3) return true;
    }
    if (op->kind == 12) {
        if (op->operand->kind == 3) return true;
    }
    return false;
}
'''
def mark(expr,v):
 return expr if v==2 else f'__builtin_expect({expr}, {v})'
def build(p):
 s=baseline
 for n,prob in zip((11,13,12),p[:3]):
  s=s.replace(f'if (op->kind == {n})',f'if ({mark(f"op->kind == {n}",prob)})')
 s=s.replace('if (op->operand->kind == 3)',f'if ({mark("op->operand->kind == 3",p[3])})')
 return header+s
modes={
'j1':['-O2','-frename-registers','-falign-jumps=1'],
'j4':['-O2','-frename-registers','-falign-jumps=4'],
'plain-j1':['-O2','-falign-jumps=1'],
'plain-j4':['-O2','-falign-jumps=4'],
'j8':['-O2','-frename-registers','-falign-jumps=8'],
'plain-j8':['-O2','-falign-jumps=8']
}
if os.environ.get('VUC_MODES'):
 modes={k:v for k,v in modes.items() if k in os.environ['VUC_MODES'].split(',')}
records=[];sizes={}
for p in itertools.product((2,0,1),repeat=4):
 key=''.join(str(i) for i in p)
 source=OUT/(key+'.cpp')
 source.write_text(build(p))
 for label,flags in modes.items():
  obj=OUT/(key+'-'+label+'.o')
  c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  if c.returncode:
   print('ERROR',key,label,c.stderr[-85:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  sym=next((s for s in nm.splitlines() if s.split()[-1:]==['_Z17IsAccumulatorablePN2IR13Operator_DataE']),'')
  size=int(sym.split()[1],16) if sym else -1
  sizes[size]=sizes.get(size,0)+1
  if size!=77:continue
  try:
   r=matcher.run(obj,ROOT/'vuc/config/ir_accumulatorable_probe.json')
   records.append((r['exact_bytes'],key,label,r['different_offsets'][:19]))
   if r['exact']:
    print('NEW_EXACT',source,label,obj,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',key,label,str(e)[:130],flush=True)
 print('DONE',key,flush=True)
print('LENGTHS',sizes,flush=True)
print('TOP')
for x in sorted(records,reverse=True)[:40]:print(x,flush=True)

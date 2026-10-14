#!/usr/bin/env python3
"""VUC accumulator classification switch source-order under GCC 3.3.3.

The original comparator sequence 11,13,12 might derive from normal switch
lowering. Test genuine case order, branch likelihood and original alignment.
"""
from pathlib import Path
import subprocess,itertools,importlib.util,collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accum-switch-guard-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
h=(R/'vuc/probes/ir_accumulatorable.cpp').read_text().split('bool IsAccumulatorable(',1)[0]
def hint(k):
 return 'op->operand->kind == 3' if k=='p' else '__builtin_expect(op->operand->kind == 3, '+k+')'
def build(order,probs,style):
 bits=[]
 for n,prob in zip(order,probs):
  if style=='cases-return':
   bits.append(f'  case {n}: if ({hint(prob)}) return true; break;\n')
  elif style=='cases-goto':
   bits.append(f'  case {n}: if ({hint(prob)}) goto yes; break;\n')
  elif style=='cases-conditional':
   bits.append(f'  case {n}: if (!({hint(prob)})) break; return true;\n')
  elif style=='cases-bool':
   bits.append(f'  case {n}: return ({hint(prob)});\n')
  else:raise AssertionError
 source='bool IsAccumulatorable(IR::Operator_Data *op)\n{\n    switch(op->kind) {\n'+''.join(bits)+'  default: break;\n  }\n  return false;\n'
 if style=='cases-goto':source+='yes: return true;\n'
 source+='}\n'
 return h+source
opts={'j8':['-O2','-frename-registers','-falign-jumps=8'],'j1':['-O2','-frename-registers','-falign-jumps=1']}
rows=[];sizes=collections.Counter()
for order,style,probs in itertools.product(itertools.permutations((11,13,12)),('cases-return','cases-goto','cases-conditional','cases-bool'),itertools.product('p01',repeat=3)):
 name='-'.join(map(str,order))+'-'+style+'-'+''.join(probs)
 cpp=T/(name+'.cpp');cpp.write_text(build(order,probs,style))
 for lab,flags in opts.items():
  obj=T/(name+'-'+lab+'.o')
  cc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if cc.returncode:
   print('COMPILE_ERROR',name,lab,cc.stderr[-80:],flush=True);continue
  nm=subprocess.check_output(['nm','-S',str(obj)],text=True)
  symbol=next((s for s in nm.splitlines() if s.split()[-1:]==['_Z17IsAccumulatorablePN2IR13Operator_DataE']),'')
  length=int(symbol.split()[1],16) if symbol else -1;sizes[length]+=1
  if length!=77:continue
  res=m.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
  rows.append((res['exact_bytes'],name,lab,res['different_offsets'][:16]))
  if res['exact']:
   print('NEW_EXACT',cpp,lab,obj,flush=True);raise SystemExit(0)
 print('CHECKED',name,flush=True)
print('LENGTHS',sizes.most_common(13))
print('TOP')
for row in sorted(rows,reverse=True)[:30]:print(row,flush=True)

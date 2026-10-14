#!/usr/bin/env python3
"""GCC 3.3.3 IsAccumulatorable: six independent real predicate probabilities.

VCL setLocks exactness arose from a conjunction of two individually neutral
probability hints. Here six real guards can receive independent hints; try
genuine branch semantics under original-era jump alignment. No assembly.
"""
import os,itertools,subprocess,importlib.util,collections
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/accum-independent-predict-oct14')
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
header=(R/'vuc/probes/ir_accumulatorable.cpp').read_text().split('bool IsAccumulatorable(',1)[0]
def hint(x,v):
 return x if v=='p' else '__builtin_expect('+x+', '+v+')'
def source(keys):
 bits=[]
 for n,outer,child in zip((11,13,12),keys[:3],keys[3:]):
  bits.append('    if ('+hint('op->kind == '+str(n),outer)+') {\n'
              +'        if ('+hint('op->operand->kind == 3',child)+') return true;\n'
              +'    }\n')
 return header+'bool IsAccumulatorable(IR::Operator_Data *op)\n{\n'+''.join(bits)+'    return false;\n}\n'
opts={
'j8':['-O2','-frename-registers','-falign-jumps=8'],
'j8plain':['-O2','-falign-jumps=8'],
'j1':['-O2','-frename-registers','-falign-jumps=1']
}
sel=os.environ.get('VUC_FLAGS','j8')
opts={k:v for k,v in opts.items() if k in sel.split(',')}
records=[];sizes=collections.Counter()
for keys in itertools.product('p01',repeat=6):
 k=''.join(keys)
 cpp=T/(k+'.cpp');cpp.write_text(source(keys))
 for lab,flags in opts.items():
  obj=T/(k+'-'+lab+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],text=True,capture_output=True)
  if cp.returncode:print('COMPILE_ERROR',k,lab,cp.stderr[-90:],flush=True);continue
  sym=next((r for r in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
            if r.split()[-1:]==['_Z17IsAccumulatorablePN2IR13Operator_DataE']),'')
  size=int(sym.split()[1],16) if sym else -1
  sizes[size]+=1
  if size!=77:continue
  try:
   x=m.run(obj,R/'vuc/config/ir_accumulatorable_probe.json')
   records.append((x['exact_bytes'],k,lab,x['different_offsets'][:18]))
   if x['exact']:print('NEW_EXACT',cpp,lab,obj,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',k,lab,str(e)[:110],flush=True)
 if keys[2:] == ('1','1','1','1'):print('DONE_OUTER',k,flush=True)
print('LENGTHS',sizes.most_common(15))
print('ORIGINAL_LENGTH',len(records))
print('TOP')
for r in sorted(records,reverse=True)[:40]:print(r,flush=True)

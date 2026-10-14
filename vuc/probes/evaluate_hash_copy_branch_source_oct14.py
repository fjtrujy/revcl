#!/usr/bin/env python3
"""Bounded historical C++ variation of VUC MLHashTable copy constructor.

Real bucket table, Node copy ctor and cleanup. Unlike alignment sweeps,
tests whether distinct outer/inner branch prediction and actual loop
induction lifetimes restore the original 166-byte body and relocations.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,collections
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/hashcopy-branchsource-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
orig=(R/'vuc/probes/hash_copy_constructor.cpp').read_text()
for header in ('MLHashTable.h', 'MLHashFunGameString.h'):
 orig=orig.replace('#include "../src/'+header+'"',
                   '#include "'+str(R/'vuc/src'/header)+'"')
outer='for (int i = 0; i < Buckets; ++i)'
inner='for (Node *node = other.heads[i]; node; node = node->next)'
assert outer in orig and inner in orig
preds={ 'plain':lambda v:v,
'likely':lambda v:'__builtin_expect('+v+',1)',
'unlikely':lambda v:'__builtin_expect('+v+',0)',
'double':lambda v:'!__builtin_expect(!('+v+'),0)',
}
variants={
 'baseline':(outer,inner),
 'while-outer':('int i=0; while(i<Buckets)',inner),
 'postinc-outer':('for (int i = 0; i < Buckets; i++)',inner),
}
flags={
'ren':['-O2','-frename-registers'],
'j1':['-O2','-frename-registers','-falign-jumps=1'],
'j2':['-O2','-frename-registers','-falign-jumps=2'],
'j4':['-O2','-frename-registers','-falign-jumps=4'],
'j8':['-O2','-frename-registers','-falign-jumps=8'],
'j16':['-O2','-frename-registers','-falign-jumps=16'],
'plain':['-O2'],
}
results=[];sizes=collections.Counter();n=0
for outer_pred,inner_pred,variant in itertools.product(preds,preds,variants):
 cpp=T/(outer_pred+'-'+inner_pred+'-'+variant+'.cpp')
 op,ip=variants[variant]
 src=orig
 if variant=='while-outer':
  src=src.replace(outer,op)
  src=src.replace('        }\n    }\n}', '        }\n        ++i;\n    }\n}')
 else:src=src.replace(outer,op)
 src=src.replace('i < Buckets',preds[outer_pred]('i < Buckets'))
 src=src.replace(inner,ip.replace('node;','('+preds[inner_pred]('node != 0')+');'))
 cpp.write_text(src)
 for name,opts in flags.items():
  obj=T/(cpp.stem+'-'+name+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  n+=1
  if p.returncode:
   print('COMPILE_ERROR',cpp.name,name,p.stderr[-100:],flush=True);continue
  line=next((s for s in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines()
    if s.endswith('_ZN11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS0_ELi513EEC1ERKS5_')),'')
  sz=int(line.split()[1],16) if line else -1;sizes[sz]+=1
  if sz!=166:continue
  try:
   x=m.run(obj,R/'vuc/config/hash_copy_constructor_probe.json')
   rec=(x['exact_bytes'],outer_pred,inner_pred,variant,name,x['different_offsets'][:10],str(cpp),str(obj))
   results.append(rec)
   if x['exact']:print('NEW_EXACT',rec,flush=True);raise SystemExit
  except ValueError as e:print('REJECT',cpp.name,name,str(e)[:100],flush=True)
 print('CHECKED',cpp.stem,'best',max((r[0] for r in results),default=0),flush=True)
print('TESTED',n,'SIZES',sizes.most_common(18),'ORIGINAL_LENGTH',len(results),flush=True)
for row in sorted(results,reverse=True)[:24]:print('TOP',row,flush=True)

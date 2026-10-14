#!/usr/bin/env python3
"""VUC 513-bucket hash-table copy ctor: original stack index and reference lives.

Original at 0x804f420 stores the outer bucket number at EBP-0x10 and
advances it there, unlike current local register induction. Check genuine
volatile/local aliases, tail owners and reference construction with period GCC.
"""
from pathlib import Path
from itertools import product
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path('/tmp/vuc-binutils215/hash-copy-ctor-index-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(R/'vuc/probes/hash_copy_constructor.cpp').read_text()
for f in ('MLHashTable.h','MLHashFunGameString.h'):
 base=base.replace('#include "../src/'+f+'"','#include "'+str(R/'vuc/src'/f)+'"')
start=base.index('    for (int i = 0; i < Buckets; ++i)')
index={
'baseline':'for (int i = 0; i < Buckets; ++i)',
'volatile':'for (volatile int i = 0; i < Buckets; ++i)',
'volatile-unsigned':'for (volatile unsigned i = 0; i < Buckets; ++i)',
'register':'for (register int i = 0; i < Buckets; ++i)',
'unsigned':'for (unsigned i = 0; i < Buckets; ++i)',
'int-while':'int i=0; while(i<Buckets)',
'volatile-while':'volatile int i=0; while(i<Buckets)',
'volatile-signed':'volatile signed i=0; while(i<Buckets)',
}
tail={
'baseline':('Node *copy = new Node(*node);','Node *copy = new Node(*node);'),
'node-ref':('Node *copy = new Node(*node);','Node &original = *node;\n            Node *copy = new Node(original);'),
'node-const-ref':('Node *copy = new Node(*node);','const Node &original = *node;\n            Node *copy = new Node(original);'),
'copy-ref':('Node *copy = new Node(*node);','Node *const &source = node;\n            Node *copy = new Node(*source);'),
'copy-pointer':('Node *copy = new Node(*node);','Node *original = node;\n            Node *copy = new Node(*original);'),
'out-ref':('Node *copy = new Node(*node);','Node **const &out = tail;\n            Node *copy = new Node(*node);'),
'out-local':('Node *copy = new Node(*node);','Node **const out = tail;\n            Node *copy = new Node(*node);'),
}
flags={'r':['-O2','-frename-registers'], 'j1':['-O2','-frename-registers','-falign-jumps=1'],
'j4':['-O2','-frename-registers','-falign-jumps=4'], 'nr':['-O2'],
'no-reorder':['-O2','-frename-registers','-fno-reorder-blocks']}
scores=[]
for (it,i),(name,(old,new)) in product(index.items(),tail.items()):
 source=base.replace('for (int i = 0; i < Buckets; ++i)',i)
 if 'while' in it:
  source=source.replace('        }\n    }\n}', '        }\n        ++i;\n    }\n}')
 source=source.replace(old,new)
 if name in ('out-ref','out-local'): source=source.replace('*tail = copy;', '*out = copy;')
 cpp=T/(it+'-'+name+'.cpp');cpp.write_text(source)
 for f,opts in flags.items():
  obj=T/(it+'-'+name+'-'+f+'.o')
  c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  if c.returncode:print('COMPILE',it,name,f,c.stderr[-85:],flush=True);continue
  sym=next((x for x in subprocess.check_output(['nm','-S',str(obj)],text=True).splitlines() if x.endswith('_ZN11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS0_ELi513EEC1ERKS5_')),'')
  size=int(sym.split()[1],16) if sym else -1
  if size!=166:scores.append((0,size,it,name,f,'length'));continue
  try:
   s=m.run(obj,R/'vuc/config/hash_copy_constructor_probe.json')
   scores.append((s['exact_bytes'],size,it,name,f,s['different_offsets'][:9]))
   if s['exact']:print('NEW_EXACT',cpp,f,flush=True);raise SystemExit(0)
  except ValueError as e:print('REJECT',it,name,f,str(e)[:120],flush=True)
 print('DONE',it,name,flush=True)
print('TOP')
for x in sorted(scores,reverse=True)[:30]:print(x,flush=True)

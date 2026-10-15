#!/usr/bin/env python3
"""Evidence-driven 16-byte Type copy order with real STL template owner.

Original _M_insert_aux at 0x804ee10 differs at the in-place copy loop
and temporary-register assignment; Type's four original 4-byte slots are
proven, but its source member order and the author's field assignments
are not. Compare actual C++03 copy-constructor body orders and assignment
orders, never manually rearrange emitted instructions.
Candidate must retain 525-byte function, nine authentic relocations, and
both accepted Type iterator-copy functions 79/79 and 86/86.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json
R=Path(__file__).resolve().parents[1];O=Path('/tmp/vuc-binutils215/type-vector-field-order-oct15t');O.mkdir(exist_ok=True)
base=(R/'probes/TypeVectorInsertAuxRealOrderOct15s.cpp').read_text()
needle='    Type(const Type &other):a(other.a),b(other.b),c(other.c),d(other.d) {}'
assert needle in base
sp=importlib.util.spec_from_file_location('matcher',R/'scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'config/type_vector_insert_aux_partial_oct15s.json'
records=[]
for domain in ('ctor','assign'):
 for order in itertools.permutations('abcd'):
  stem=''.join(' '+c+'=other.'+c+';' for c in order)
  if domain=='ctor':replacement='    Type(const Type &other) {'+stem+' }'
  else:replacement=needle+'\n    Type &operator=(const Type &other) {'+stem+' return *this; }'
  src=base.replace(needle,replacement)
  name=domain+'-'+''.join(order)
  cpp=O/(name+'.cpp');cpp.write_text(src)
  obj=O/(name+'.o')
  p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215','-O2','-frename-registers','-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
  item={'kind':domain,'order':''.join(order)}
  if p.returncode:item['error']=p.stderr[-180:]
  else:
   try:
    v=m.run(obj,cfg);item['score']=v['exact_bytes'];item['length']=v['length'];item['relocations']=v['relocations']
    for key,c in [('const','uninitialized_type_const_probe.json'),('mut','uninitialized_type_mut_probe.json')]:
     vv=m.run(obj,R/'config'/c);item[key]=vv['exact_bytes']
   except ValueError as e:item['error']=str(e)
  records.append(item)
  print(item,flush=True)
(O/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print('RANKED',sorted([r for r in records if 'score' in r],key=lambda r:r['score'],reverse=True)[:12],flush=True)

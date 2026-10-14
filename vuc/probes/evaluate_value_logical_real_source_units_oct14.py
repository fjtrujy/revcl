#!/usr/bin/env python3
"""Reconstruct authentic historical Value source-unit predecessors for &&.

Compile only real C++03 functions present in the VUC ELF, in their actual
symbol order, rather than dummy predecessors, register pinning or padding.
Probe register/peephole state for both logical sibling functions, retaining
original 325/309 function-length and TypeConvert relocation as hard checks.
"""
from pathlib import Path
import importlib.util,subprocess,json,itertools,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cmp=(R/'vuc/src/ValueComparison.cpp').read_text()
and_s=(R/'vuc/probes/value_logical_and_partial_oct14.cpp').read_text()
or_s=(R/'vuc/src/ValueLogicalOr.cpp').read_text()
head=and_s[:and_s.index('Value Value::operator&&')]
and_body=and_s[and_s.index('Value Value::operator&&'):]
or_body=or_s[or_s.index('Value Value::operator||'):]
comp_decl=cmp.replace('    Value operator<=(const Value &other) const;', '    Value operator<=(const Value &other) const;\n    Value operator&&(const Value &other) const;\n    Value operator||(const Value &other) const;')
assert comp_decl != cmp
def full(combo):
 if combo.startswith('comparison-'):
  return comp_decl+'\n'+ ('\n'.join({'and':and_body,'or':or_body}[t] for t in combo.split('-')[1:]))+'\n'
 return head+'\n'.join({'and':and_body,'or':or_body}[t] for t in combo.split('-'))+'\n'
targets={
 'and':json.loads((R/'vuc/config/value_logical_and_partial_oct14_probe.json').read_text()),
 'or':json.loads((R/'vuc/config/value_logical_or_exact_oct14_probe.json').read_text()),
}
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
ref={}
for label,c in targets.items():
 addr=int(c['address'],0)
 ref[label]=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+c['size']]
T=Path('/tmp/vuc-binutils215/value-real-comparison-logical-tu-batch');T.mkdir(parents=True,exist_ok=True)
recipes=('and','and-or','or-and','comparison-and','comparison-and-or','comparison-or-and')
opts={
 'and16':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=1','-falign-loops=1'],
 'and16-l8':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=8','-falign-loops=1'],
 'and16-l16':['-O2','-frename-registers','-ffast-math','-falign-jumps=16','-falign-labels=16','-falign-loops=1'],
 'or8':['-O2','-frename-registers','-ffast-math','-falign-jumps=8','-falign-labels=1','-falign-loops=4'],
 'and8':['-O2','-frename-registers','-ffast-math','-falign-jumps=8','-falign-labels=1','-falign-loops=1'],
 'and4':['-O2','-frename-registers','-ffast-math','-falign-jumps=4','-falign-labels=8','-falign-loops=1'],
 'plain':['-O2','-frename-registers','-ffast-math']
}
records=[];best={}
for (group,label),snap in itertools.product(itertools.product(recipes,opts),('jan25','dec06')):
 key=f'{group}-{label}-{snap}';source=full(group)
 cpp=T/(key+'.cpp');obj=T/(key+'.o');cpp.write_text(source)
 c=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215',*opts[label],'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if c.returncode:
  records.append(dict(key=key,error=c.stderr[-200:]));continue
 r=dict(key=key)
 for name,config in targets.items():
  if name not in group:continue
  addr=int(config['address'],0)
  try:
   data,rel=m.prepare_named_function(obj.read_bytes(),config['function'],config['relocations'],addr)
  except ValueError as e:
   r[name]=dict(error=str(e));continue
  count=sum(a==b for a,b in zip(data,ref[name]))
  prefix=next((i for i,(a,b) in enumerate(zip(data,ref[name])) if a!=b),min(len(data),len(ref[name])))
  score=dict(length=len(data),relocations=rel,overlap=count,prefix=prefix,exact=len(data)==config['size'] and count==len(data) and rel==1)
  r[name]=score
  if len(data)==config['size'] and rel==1 and (name not in best or (count,prefix)>(best[name]['overlap'],best[name]['prefix'])):
   best[name]=dict(key=key,**score);(T/('best-'+name+'.cpp')).write_text(source);(T/('best-'+name+'.o')).write_bytes(obj.read_bytes());(T/('best-'+name+'.flags')).write_text(' '.join(opts[label])+'\n')
 records.append(r)
(T/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps(dict(attempted=len(records),best=best,top_and=sorted((r for r in records if 'and' in r and 'overlap'in r['and']),key=lambda r:r['and']['overlap'],reverse=True)[:10],top_or=sorted((r for r in records if 'or' in r and 'overlap' in r['or']),key=lambda r:r['or']['overlap'],reverse=True)[:7]),indent=2))

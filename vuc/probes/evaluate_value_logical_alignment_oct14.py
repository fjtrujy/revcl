#!/usr/bin/env python3
"""Probe original-size GCC alignment of real Value && / || sources.

Both Value logical operations can be reconstructed semantically with
TypeConvert and native integer/x87 comparisons. Distinguish original-length
instruction matches from high-overlap wrong-sized control-flow layouts.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2];T=Path('/tmp/vuc-binutils215/value-logical-alignment-next-oct14');T.mkdir(exist_ok=True,parents=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
inputs={
 'and':(R/'vuc/probes/evaluate_value_logical_reconstruct_oct14.py',
        Path('/tmp/vuc-binutils215/value-logical-reconstruct-next-oct14/and-truth-neq-chained-ffast.cpp'),
        '_ZNK5ValueaaERKS_',0x0806f340,325),
 'or':(R/'vuc/probes/evaluate_value_logical_reconstruct_oct14.py',
       Path('/tmp/vuc-binutils215/value-logical-reconstruct-next-oct14/best-or.cpp'),
       '_ZNK5ValueooERKS_',0x0806f490,309),
}
rel={'_ZN5Value11TypeConvertERKS_S1_RS_S2_':'0x0806f7d0'}
results={}
for logical,(producer,source,symbol,addr,length) in inputs.items():
 ref=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+length]
 records=[];best=None;sizes=collections.Counter()
 for jump,label,loop,reorder in itertools.product((1,2,4,8),(1,2,4,8),(1,4),(True,False)):
  tag=f'{logical}-j{jump}-l{label}-loop{loop}-'+('reorder' if reorder else 'no-reorder')
  opts=['-O2','-frename-registers','-ffast-math',f'-falign-jumps={jump}',f'-falign-labels={label}',f'-falign-loops={loop}']
  if not reorder:opts+=['-fno-reorder-blocks']
  obj=T/(tag+'.o')
  cp=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(source),'-o',str(obj)],capture_output=True,text=True)
  if cp.returncode:
   records.append(dict(tag=tag,error=cp.stderr[-150:]));continue
  try:code,r=m.prepare_named_function(obj.read_bytes(),symbol,rel,addr)
  except ValueError as e:
   records.append(dict(tag=tag,error=str(e)));continue
  score=sum(x==y for x,y in zip(code,ref))
  prefix=next((i for i,(x,y) in enumerate(zip(code,ref)) if x!=y),min(len(code),len(ref)))
  row=dict(tag=tag,length=len(code),relocations=r,overlap=score,prefix=prefix,flags=opts)
  records.append(row);sizes[len(code)]+=1
  if len(code)==length and r==1 and (best is None or (score,prefix)>(best['overlap'],best['prefix'])):
   best=row
   (T/('best-'+logical+'.o')).write_bytes(obj.read_bytes())
   (T/('best-'+logical+'.flags')).write_text(' '.join(opts)+'\n')
 results[logical]=dict(attempted=len(records),sizes=dict(sizes),best_original_length=best,top=sorted((x for x in records if 'overlap'in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:7])
 (T/(logical+'-results.json')).write_text(json.dumps(records,indent=2)+'\n')
 print(json.dumps({logical:results[logical]},indent=2),flush=True)

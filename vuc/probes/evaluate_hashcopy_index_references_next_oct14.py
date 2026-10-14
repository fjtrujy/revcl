#!/usr/bin/env python3
"""Bounded VUC MLHashTable copy-constructor index aliases (historical C++03).

Original 0x0804f420 uses a stack-resident bucket index and a live linked-list
tail. The earlier volatile-index matrix lengthened EH code; this tests actual
C++ lvalue/pointer aliases without volatile stores or register constraints.
Only 166-byte / four authenticated relocations can become experimental.
"""
from pathlib import Path
import importlib.util,itertools,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/hash_copy_constructor_probe.json').read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text')
addr=int(cfg['address'],0);orig=raw[sec['offset']+addr-sec['address']:sec['offset']+addr-sec['address']+166]
src=(R/'vuc/probes/hash_copy_constructor.cpp').read_text()
for f in ('MLHashTable.h','MLHashFunGameString.h'):
 src=src.replace('#include "../src/'+f+'"','#include "'+str(R/'vuc/src'/f)+'"')
where='    for (int i = 0; i < Buckets; ++i) {\n'
assert where in src
aliases={
 'direct':('', 'i'),
 'ref':('        int &bucket = i;\n','bucket'),
 'constref':('        const int &bucket = i;\n','bucket'),
 'ptr':('        int *const bucket = &i;\n','*bucket'),
 'constptr':('        const int *const bucket = &i;\n','*bucket'),
 'copy':('        int bucket = i;\n','bucket'),
 'ucopy':('        unsigned bucket = i;\n','bucket'),
}
flags={
 'base':['-O2','-frename-registers'],
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'nr':['-O2','-frename-registers','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/hashcopy-index-refs-next-oct14');T.mkdir(parents=True,exist_ok=True)
rows=[];sizes=collections.Counter();best=None
for (name,(declaration,index)),(flag,opts),snap in itertools.product(aliases.items(),flags.items(),('jan25','dec06')):
 tag=name+'-'+flag+'-'+snap
 variant=src.replace(where,where+declaration)
 variant=variant.replace('heads[i]',f'heads[{index}]')
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(variant)
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+snap+'-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:
  rows.append(dict(name=tag,error=cp.stderr[:140]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:
  rows.append(dict(name=tag,error=str(e)));continue
 score=sum(a==b for a,b in zip(code,orig))
 prefix=next((i for i,(a,b) in enumerate(zip(code,orig)) if a!=b),min(len(code),len(orig)))
 row=dict(name=tag,length=len(code),relocations=rel,overlap=score,prefix=prefix)
 rows.append(row);sizes[len(code)]+=1
 if len(code)==166 and rel==4 and (best is None or (score,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (T/'best.cpp').write_text(cpp.read_text());(T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best_original_length=best,top=sorted((x for x in rows if 'overlap' in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:8]),indent=2))

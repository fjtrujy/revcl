#!/usr/bin/env python3
"""VUC original 149-byte STL normal_iterator uninitialized_copy exception helper.

GCC3.3.3 native libstdc++ specialization compiles 136/142 bytes with all
six authentic call references. Prior bounded probes tested jump alignment
up to 8. Test historical jump/loop/label choices up to 32 while retaining
unchanged standard C++03 source and documented clear() throw() declaration.
Wrong-sized candidates never enter manifests.
"""
from pathlib import Path
import itertools,json,subprocess,importlib.util,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
config=json.loads((R/'vuc/config/scope_iterator_copy_oct12_probe.json').read_text())
src=(R/'vuc/probes/scope_iterator_clear_throw_oct14.cpp').read_text()
src=src.replace('#include "../src/MLHashFunGameString.h"','#include "'+str(R/'vuc/src/MLHashFunGameString.h')+'"')
src=src.replace('#include "../src/ScopeCopyTable.h"','#include "'+str(R/'vuc/src/ScopeCopyTable.h')+'"')
T=Path('/tmp/vuc-binutils215/scope-iterator-large-alignment-oct15e');T.mkdir(exist_ok=True,parents=True)
raw,elf=m.decomp.load();sec=next(s for s in elf['sections'] if s['name']=='.text');a=int(config['address'],0);ref=raw[sec['offset']+a-sec['address']:sec['offset']+a-sec['address']+149]
rows=[];counts=collections.Counter();best=None
for jump,label,loop,rename in itertools.product((1,2,4,8,16,32),(1,2,4,8,16),(1,4), (False,True)):
 tag=f'j{jump}-l{label}-loop{loop}-{"r" if rename else "p"}'
 flags=['-O2']+(['-frename-registers'] if rename else [])+[f'-falign-jumps={jump}',f'-falign-labels={label}',f'-falign-loops={loop}']
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(src)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-120:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),config['function'],config['relocations'],a)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:160]));continue
 score=sum(x==y for x,y in zip(ref,b));px=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=tag,length=len(b),match=score,prefix=px,relocations=r)
 rows.append(row);counts[len(b)]+=1
 if len(b)==149 and r==6 and (best is None or (score,px)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(src);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags)+'\n')
  print('STRICT-ORIGINAL-LENGTH',json.dumps(row),flush=True)
  if score==149:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(counts),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:14]),indent=2))

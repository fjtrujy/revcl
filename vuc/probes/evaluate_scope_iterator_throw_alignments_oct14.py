#!/usr/bin/env python3
"""Real C++03 STL normal_iterator scope copy + authenticated clear throw():
test GCC basic-block layout without artificial NOPs or code mutations.
"""
from pathlib import Path
import itertools,importlib.util,subprocess,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/scope_iterator_copy_oct12_probe.json").read_text())
raw,elf=m.decomp.load();sec=next(s for s in elf["sections"] if s["name"]==".text")
addr=int(cfg["address"],0)
orig=raw[sec["offset"]+addr-sec["address"]:sec["offset"]+addr-sec["address"]+cfg["size"]]
src=R/"vuc/probes/scope_iterator_clear_throw_oct14.cpp"
out=Path("/tmp/vuc-binutils215/scope-iterator-alignment-throw-oct14");out.mkdir(exist_ok=True,parents=True)
rows=[];sizes=collections.Counter();best=None
for compiler,jump,label,loop,reorder in itertools.product(
 ('jan25','dec06'),(1,4,8,16),(1,2,4,8),(1,2,4,8),(True,False)):
 name=f'{compiler}-j{jump}-l{label}-loop{loop}-'+('nr' if reorder else 'r')
 flags=['-O2','-frename-registers',f'-falign-jumps={jump}',
        f'-falign-labels={label}',f'-falign-loops={loop}']
 if reorder:flags+=['-fno-reorder-blocks']
 obj=out/(name+'.o')
 cp=subprocess.run(['/tmp/vuc-binutils215/g++-'+compiler+'-as215',*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
 if cp.returncode:
  rows.append(dict(name=name,error=cp.stderr[-120:]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:
  rows.append(dict(name=name,error=str(e)));continue
 size=len(code);sizes[size]+=1
 overlap=sum(a==b for a,b in zip(code,orig))
 prefix=next((i for i,(a,b) in enumerate(zip(code,orig)) if a!=b),min(size,len(orig)))
 row=dict(name=name,length=size,relocations=rel,overlap=overlap,prefix=prefix)
 rows.append(row)
 if size==149 and rel==6 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (out/'best.o').write_bytes(obj.read_bytes())
  (out/'best.flags').write_text(compiler+' '+' '.join(flags)+'\n')
(out/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(sizes),best_original_length=best,highest_original_length=sorted((x for x in rows if x.get('length')==149),key=lambda x:x['overlap'],reverse=True)[:8],high_overlap=sorted((x for x in rows if 'overlap'in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:6]),indent=2))

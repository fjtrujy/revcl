#!/usr/bin/env python3
"""VUC string-fragment-list continuation after authentic index-lifetime gain.

Previous script established a 306-byte / 75-byte source by incrementing
the list index between fragment construction and Add(). This isolates the
actual output-list growth predicate and old-storage copy predicate to learn
how GCC 3.3.3 chose their branches. No hand-written opcode/register.
"""
from pathlib import Path
import itertools,importlib.util,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/"vuc/config/translate_frag_list_string_oct14_probe.json").read_text())
raw,elf=m.decomp.load();section=next(x for x in elf['sections'] if x['name']=='.text')
addr=int(cfg['address'],0)
ref=raw[section['offset']+addr-section['address']:section['offset']+addr-section['address']+306]
base=Path("/tmp/vuc-binutils215/string-fraglist-index-lifetime-next-oct14/best.cpp").read_text()
assert 'if (old_size >= capacity)' in base and 'if (old_entries)' in base
modes={
 'plain':lambda x:x,
 'likely':lambda x:'__builtin_expect(('+x+'),1)',
 'unlikely':lambda x:'__builtin_expect(('+x+'),0)',
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j2':['-O2','-frename-registers','-falign-jumps=2'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j1-nr':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
 'j2-nr':['-O2','-frename-registers','-falign-jumps=2','-fno-reorder-blocks'],
 'j4-nr':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'j8-nr':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
}
T=Path('/tmp/vuc-binutils215/string-fraglist-index-growth-hints-oct14')
T.mkdir(parents=True,exist_ok=True)
rows=[];counts=collections.Counter();best=None
for (gname,g),(cname,c),fl in itertools.product(modes.items(),modes.items(),flags):
 name=f'{gname}-{cname}-{fl}'
 src=T/(name+'.cpp');obj=T/(name+'.o')
 code=base.replace('if (old_size >= capacity)','if ('+g('old_size >= capacity')+')')
 code=code.replace('if (old_entries)','if ('+c('old_entries != 0')+')')
 src.write_text(code)
 proc=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags[fl],'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
 if proc.returncode:
  rows.append(dict(name=name,error=proc.stderr[:180]));continue
 try:body,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:
  rows.append(dict(name=name,error=str(e)));continue
 n=len(body);counts[n]+=1
 overlap=sum(a==b for a,b in zip(body,ref))
 prefix=next((i for i,(a,b) in enumerate(zip(body,ref)) if a!=b),min(n,len(ref)))
 row=dict(name=name,length=n,relocations=rel,overlap=overlap,prefix=prefix)
 rows.append(row)
 if n==306 and rel==6 and (best is None or (overlap,prefix)>(best['overlap'],best['prefix'])):
  best=row
  (T/'best.cpp').write_text(code)
  (T/'best.o').write_bytes(obj.read_bytes())
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),sizes=dict(counts),best_original_length=best,top=sorted((x for x in rows if 'overlap'in x),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:12]),indent=2))

#!/usr/bin/env python3
"""GCC 3.3.3 loop-entry/return lifetime controls for the VUC 306-byte string fragment list.
Unaccepted experiments remain scratch-only; each comparison uses the ZIP-backed ELF.
"""
from pathlib import Path
import importlib.util,itertools,json,subprocess,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('match',R/'vuc/scripts/match_probe.py')
m=importlib.util.module_from_spec(sp); sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/translate_frag_list_string_oct14_probe.json').read_text())
raw,target=m.decomp.load()
sec=next(s for s in target['sections'] if s['name']=='.text')
addr=int(cfg['address'],0)
p=sec['offset']+addr-sec['address']
orig=raw[p:p+cfg['size']]
base=(R/cfg['source']).read_text().replace('#include "../src/MLGameString.h"', '#include "'+str(R/'vuc/src/MLGameString.h')+'"')
first=base.index('    while (__builtin_expect((i < input.length), 1)) {')
last=base.index('    return output;',first)
content=base[first:last]
inner=content[content.index('{')+1:content.rfind('}')]
prefix=base[:first];suffix=base[last:]
conditions={'plain':'i < input.length','likely':'__builtin_expect((i < input.length), 1)','unlikely':'__builtin_expect((i < input.length), 0)'}
forms={
 'while': lambda c:'    while ('+c+') {'+inner+'    }\n',
 'for': lambda c:'    for (; '+c+';) {'+inner+'    }\n',
 'guard-do': lambda c:'    if ('+c+') do {'+inner+'    } while ('+c+');\n',
 'return-do': lambda c:'    if (!('+c+')) return output;\n    do {'+inner+'    } while ('+c+');\n',
 'guard-while':lambda c:'    if ('+c+') while ('+c+') {'+inner+'    }\n',
 'return-while':lambda c:'    if (!('+c+')) return output;\n    while ('+c+') {'+inner+'    }\n',
 'do-break':lambda c:'    do {\n        if (!('+c+')) break;'+inner+'    } while (true);\n',
 'goto-exit':lambda c:'    if (!('+c+')) goto done;\n    do {'+inner+'    } while ('+c+');\ndone:\n',
}
options={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j2':['-O2','-frename-registers','-falign-jumps=2'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j4-nr':['-O2','-frename-registers','-falign-jumps=4','-fno-reorder-blocks'],
 'j1-nr':['-O2','-frename-registers','-falign-jumps=1','-fno-reorder-blocks'],
 'j8-nr':['-O2','-frename-registers','-falign-jumps=8','-fno-reorder-blocks'],
 'j4-label':['-O2','-frename-registers','-falign-jumps=4','-falign-labels=1'],
}
out=Path('/tmp/vuc-binutils215/string-fraglist-loop-structure-oct14')
out.mkdir(parents=True,exist_ok=True)
records=[];lengths=collections.Counter();best=None
for (name,form),(kind,condition),(flag,flags) in itertools.product(forms.items(),conditions.items(),options.items()):
 label=f'{name}-{kind}-{flag}'
 src=out/(label+'.cpp'); obj=out/(label+'.o')
 src.write_text(prefix+form(condition)+suffix)
 result=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(src),'-o',str(obj)],capture_output=True,text=True)
 if result.returncode:
  records.append(dict(label=label,error=result.stderr[:180]));continue
 try:code,rel=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],addr)
 except ValueError as e:
  records.append(dict(label=label,error=str(e)));continue
 n=len(code);lengths[n]+=1
 overlap=sum(a==b for a,b in zip(code,orig))
 prefix_match=next((i for i,(a,b) in enumerate(zip(code,orig)) if a!=b),min(n,len(orig)))
 row=dict(label=label,length=n,relocations=rel,overlap=overlap,prefix=prefix_match)
 records.append(row)
 if n==len(orig) and rel==6 and (best is None or (overlap,prefix_match)>(best['overlap'],best['prefix'])):
  best=row
  (out/'best.cpp').write_text(src.read_text())
  (out/'best.o').write_bytes(obj.read_bytes())
(out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(json.dumps({'attempted':len(records),'lengths':dict(lengths),'best_exact_size':best,'best_overlap':sorted((r for r in records if 'overlap'in r),key=lambda x:(x['overlap'],x['prefix']),reverse=True)[:8],'result_path':str(out/'results.json')},indent=2))

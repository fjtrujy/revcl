#!/usr/bin/env python3
"""VUC GetNextWord: nearby authentic signed-byte branch-tree source hypotheses.

Starting from newly original-length 36/135 original-offset partial,
test semantically equivalent C++03 nested low/high delimiter decisions
and historical GCC3.3.3 alignment. No synthetic register assignments,
inline assembler, filler, fake relocations or source dead padding.
"""
from pathlib import Path
import importlib.util,json,itertools,collections,subprocess
R=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
config=R/'vuc/config/get_next_word_partial_oct15e_probe.json'
base=(R/'vuc/probes/GetNextWordRange36BaselineOct15f.cpp').read_text()
a=base.index('if(c==44)goto endword;');b=base.index('} *dest++=c;',a)+1
pred=base[a:b]
assert 'if(c>44)' in pred and 'if(c>9)' in pred
variants={
 'baseline':pred,
 'low-first':'''if(c==44)goto endword;
if(c<44) {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;
} else {
 if(c==46)goto endword;
 if(c==91)goto endword;
}''',
 'low-le':'''if(c==44)goto endword;
if(c<=44) {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;
} else {
 if(c==46)goto endword;
 if(c==91)goto endword;
}''',
 'high-combined':'''if(c==44)goto endword;
if(c>44) {
 if(c==46 || c==91)goto endword;
} else {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;
}''',
 'low-combined':'''if(c==44)goto endword;
if(c>44) {
 if(c==46)goto endword; if(c==91)goto endword;
} else {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;
}''',
 'low-zero-first':'''if(c==44)goto endword;
if(c>44) {
 if(c==46)goto endword; if(c==91)goto endword;
} else {
 if(c==0)goto endword;
 if(c==9)goto endword;
 if(c==32)goto endword;
}''',
 'low-nested':'''if(c==44)goto endword;
if(c>44) {
 if(c==46)goto endword; if(c==91)goto endword;
} else {
 if(c==9)goto endword;
 if(c<=9){if(c==0)goto endword;}else if(c==32)goto endword;
}''',
 'low-bound-switch':'''if(c==44)goto endword;
if(c>44) {switch(c){case 46:case 91:goto endword;}}
else {switch(c){case 0:case 9:case 32:goto endword;}}''',
 'low-ternary':'''if(c==44)goto endword;
if(c>44) {if(c==46)goto endword; if(c==91)goto endword;}
else {if(c==9)goto endword; if(c>9){if(c==32)goto endword;}else{if(!c)goto endword;}}''',
 'negative-44':'''if(c!=44)goto other;
goto endword;
other:
if(c>44) {
 if(c==46)goto endword; if(c==91)goto endword;
} else {
 if(c==9)goto endword;
 if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;
}''',
 'negative-high':'''if(c==44)goto endword;
if(c<=44)goto lower;
if(c==46)goto endword;
if(c==91)goto endword;
goto nextbyte;
lower:
if(c==9)goto endword;
if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;
nextbyte:;''',
 'separate-range':'''if(c==44)goto endword;
if(c>44) {if(c==46)goto endword; if(c==91)goto endword;}
if(c<44) {if(c==9)goto endword; if(c>9){if(c==32)goto endword;}else if(c==0)goto endword;}''',
 'exact-case-bounds':'''if(c==44)goto endword;
if(c>=45) {
 if(c==46)goto endword; if(c==91)goto endword;
} else {
 if(c==9)goto endword;
 if(c>=10){if(c==32)goto endword;}else if(c==0)goto endword;
}''',
}
T=Path('/tmp/vuc-binutils215/getnext-range-tree-topology-oct15f');T.mkdir(parents=True,exist_ok=True)
rank=[];sizes=collections.Counter();best=None
for (name,cond),(jump,label,loop) in itertools.product(variants.items(),itertools.product((4,8,16),(1,2,4),(1,2,4))):
 tag=f'{name}-j{jump}-label{label}-loop{loop}'
 opts=['-O2','-frename-registers',f'-falign-jumps={jump}',f'-falign-labels={label}',f'-falign-loops={loop}']
 code=base[:a]+cond+base[b:]
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(code)
 r=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if r.returncode:rank.append(dict(tag=tag,error=r.stderr[-110:]));continue
 try:d=m.run(obj,config)
 except ValueError as e:rank.append(dict(tag=tag,error=str(e)[:145]));continue
 rec=dict(tag=tag,length=d['length'],match=d['exact_bytes'],relocations=d['relocations'],prefix=next(iter(d['different_offsets']),d['length']))
 rank.append(rec);sizes[d['length']]+=1
 if rec['length']==135 and rec['relocations']==5 and (best is None or (rec['match'],rec['prefix'])>(best['match'],best['prefix'])):
  best=rec;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
  if rec['match']>36:print('IMPROVEMENT',json.dumps(rec),flush=True)
  if rec['match']==135:print('EXACT',json.dumps(rec),flush=True);break
(T/'results.json').write_text(json.dumps(rank,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rank),sizes=dict(sizes),strict=sum(x.get('length')==135 and x.get('relocations')==5 for x in rank),best=best,top=sorted((x for x in rank if 'match' in x),key=lambda x:(x['match'],x['prefix']),reverse=True)[:14]),indent=2))

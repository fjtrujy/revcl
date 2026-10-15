#!/usr/bin/env python3
"""VUC 50/135 GetNextWord source lifetime and return-value refinements.

The original original-length nested signed-range C++03 scanner matches 50
instruction bytes with original relocations. Test natural typed positions,
parameter hint, exact same delimiter semantics and increment ownership to
recover original EAX(position)/ECX(input) allocator preference.
No assembly, fictitious branches, volatile dead fillers, or register pins.
"""
from pathlib import Path
import itertools,json,importlib.util,subprocess,collections,random
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/get_next_word_partial_oct15e_probe.json'
base=(R/'vuc/probes/GetNextWordNestedRangePartialOct15f.cpp').read_text()
original_p='int position=currentPosition;'
original_skip='++position; ++input; currentPosition=position;'
original_write='*dest++=c;\n ++input;\n ++position;\n currentPosition=position;'
original_final='*dest=0;\n return input;'
assert all(x in base for x in (original_p,original_skip,original_write,original_final))
decl={
 'int':'int position=currentPosition;',
 'regint':'register int position=currentPosition;',
 'unsigned':'unsigned int position=(unsigned int)currentPosition;',
 'long':'long position=currentPosition;',
 'constref':'int position=(int&)currentPosition;',
}
first={
 'unsigned':'const unsigned char c = *input;',
 'char':'const char c = *input;',
 'signed':'const signed char c = *input;',
 'int':'const int c = (unsigned char)*input;',
}
skips={
 'original':original_skip,
 'input-first':'++input; ++position; currentPosition=position;',
 'global++':'++input; currentPosition=++position;',
 'p-store':'currentPosition=++position; ++input;',
 'post':'position++; input++; currentPosition=position;',
}
write={
 'original':original_write,
 'input-first':'*dest++=c;\n ++input;\n currentPosition=++position;',
 'pos-first':'*dest++=c;\n ++position;\n ++input;\n currentPosition=position;',
 'char-pointer':'*dest=c;\n ++input;\n currentPosition=++position; ++dest;',
}
end={
 'original':original_final,
 'saved-before':'const char *ret=input; *dest=0; return ret;',
 'saved-after':'*dest=0; const char *ret=input; return ret;',
 'comma':'return ((*dest=0),input);',
}
flags={
 'base':['-O2','-frename-registers','-falign-jumps=16','-falign-labels=1','-falign-loops=2'],
 'jump8':['-O2','-frename-registers','-falign-jumps=8','-falign-labels=1','-falign-loops=2'],
 'plain':['-O2','-falign-jumps=16','-falign-labels=1','-falign-loops=2'],
}
T=Path('/tmp/vuc-binutils215/getnext-50-sampled-lifetimes-oct15f');T.mkdir(parents=True,exist_ok=True)
rows=[];lengths=collections.Counter();best=None
recipes=list(itertools.product(decl,first,skips,write,end,flags))
# Sample all genuine source families, instead of only the first declaration
# family from a lexicographically truncated Cartesian product.
random.Random(20041015).shuffle(recipes)
for d,f,s,w,e,opt in recipes[:480]:
 name='-'.join((d,f,s,w,e,opt))
 code=base.replace(original_p,decl[d]).replace('const unsigned char c = *input;',first[f])
 code=code.replace(original_write,'WRITE_CONTROL')
 code=code.replace(original_skip,skips[s])
 code=code.replace('WRITE_CONTROL',write[w]).replace(original_final,end[e])
 options=flags[opt]
 cpp=T/(name+'.cpp');obj=T/(name+'.o');cpp.write_text(code)
 run=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*options,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if run.returncode:rows.append(dict(tag=name,error=run.stderr[-120:]));continue
 try:rs=m.run(obj,cfg)
 except ValueError as exc:rows.append(dict(tag=name,error=str(exc)[:125]));continue
 rec=dict(tag=name,length=rs['length'],match=rs['exact_bytes'],relocations=rs['relocations'],prefix=next(iter(rs['different_offsets']),rs['length']))
 rows.append(rec);lengths[rs['length']]+=1
 if rec['length']==135 and rec['relocations']==5 and (best is None or (rec['match'],rec['prefix'])>(best['match'],best['prefix'])):
  best=rec;(T/'best.cpp').write_text(code);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(options)+'\n')
  if rec['match']>50:print('IMPROVE',json.dumps(rec),flush=True)
  if rec['match']==135:print('EXACT',json.dumps(rec),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),original_sized=sum(x.get('length')==135 and x.get('relocations')==5 for x in rows),lengths=dict(lengths),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda r:(r['match'],r['prefix']),reverse=True)[:9]),indent=2))

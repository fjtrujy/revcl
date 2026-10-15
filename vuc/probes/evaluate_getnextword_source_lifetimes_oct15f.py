#!/usr/bin/env python3
"""VUC GetNextWord register lifetime, data ownership and position-update sweep.

Source-level 2004-era C++03 expression/lifetime variants. This specifically
targets the original EAX=currentPosition / ECX=input ordering without register
bindings, inline assembler, padding, or patched machine code. Exact original
size, relocations and literal provenance are required for selectable progress.
"""
from pathlib import Path
import itertools,subprocess,importlib.util,json,collections
R=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=R/'vuc/config/get_next_word_partial_oct15e_probe.json'
preamble='extern int currentPosition;\nextern int wordPosition;\nextern char word[1024];\n'
leading={
 'current':'for (;;) { const unsigned char c=*scan; if(c<=10) {if(c<9)break;} else if(c!=32)break; SKIP }',
 'inverted':'for (;;) { const unsigned char c=*scan; if(c>10) {if(c!=32)break;} else if(c<9)break; SKIP }',
 'goto':'for (;;) { const unsigned char c=*scan; if(c>10)goto above; if(c<9)goto endlead;goto white;above:if(c!=32)goto endlead;white: SKIP }endlead:;',
 'range':'for (;;) { unsigned char c=*scan; if (c<9 || (c>10 && c!=32))break; SKIP }',
 'int-range':'for (;;) { int c=(unsigned char)*scan; if(c<=10) {if(c<9)break;} else if(c!=32)break; SKIP }',
 'while':'while ((*scan == 9) || (*scan == 10) || (*scan == 32)) { SKIP }',
}
skips={
 'pos-then-input':'++position; ++scan; currentPosition=position;',
 'input-then-pos':'++scan; ++position; currentPosition=position;',
 'pos-before-global':'currentPosition=++position; ++scan;',
 'input-prefix-global':'++scan; currentPosition=++position;',
 'post-prefix':'position++; scan++; currentPosition=position;',
 'post-assignment':'++scan; currentPosition=(position+=1);',
}
wordbody={
 'plain':'unsigned char c=*scan; if(c==0||c==9||c==32||c==44||c==46||c==91) goto endword; *dest++=c; ++scan; ++position; currentPosition=position;',
 'update-first':'unsigned char c=*scan; if(c==0||c==9||c==32||c==44||c==46||c==91) goto endword; *dest=c; ++scan; currentPosition=++position; ++dest;',
 'pos-first':'unsigned char c=*scan; if(c==0||c==9||c==32||c==44||c==46||c==91) goto endword; *dest++=c; ++position; ++scan; currentPosition=position;',
 'direct-global':'unsigned char c=*scan; if(c==0||c==9||c==32||c==44||c==46||c==91) goto endword; *dest++=c; ++scan; currentPosition=++position;',
}
declarations={
 'base':'int position=currentPosition; const char *scan=input;',
 'regposition':'register int position=currentPosition; const char *scan=input;',
 'unsigned':'unsigned position=currentPosition; const char *scan=input;',
 'long':'long position=currentPosition; const char *scan=input;',
 'regpointer':'int position=currentPosition; register const char *scan=input;',
 'regboth':'register int position=currentPosition; register const char *scan=input;',
 'scanahead':'const char *scan=input; int position=currentPosition;',
 'pointernamed':'int position=currentPosition; const char * const first=input; const char *scan=first;',
 'pos-reference':'int position=(int&)currentPosition; const char *scan=input;',
 'from-global':'int position=+currentPosition; const char *scan=input;',
 'unsigned-lifetime':'unsigned int position=(unsigned int)currentPosition; const char *scan=input;',
 'output-ahead':'int position=currentPosition; const char *scan=input; char *dest=word;',
 'output-first':'char *dest=word; int position=currentPosition; const char *scan=input;',
}
body='''const char *GetNextWord(const char *input) {
 DECLARATION
 LEADING
 wordPosition=position;
 OUTPUT_DECL
 for (;;) {
  WORDBODY
 }
endword:
 *dest=0;
 return scan;
}
'''
flags={
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
}
# Cover smaller but diverse base/guard/update/ownership combinations before
# a larger Cartesian product; fixed, deterministic and reproducible.
recipes=[]
for d,l,up,w in itertools.product(declarations,leading,skips,wordbody):
 if len(recipes)>=650:break
 # Ensure first batch covers core combinations across all meaningful families.
 if d not in ('base','regposition','unsigned','regpointer','scanahead','output-ahead','output-first') and l not in ('current','inverted'):continue
 if w!='plain' and up not in ('input-then-pos','input-prefix-global'):continue
 for fl in ('j16','j4'):
  recipes.append((d,l,up,w,fl))
  if len(recipes)>=650:break
T=Path('/tmp/vuc-binutils215/getnextword-source-lifetime-next-oct15f');T.mkdir(exist_ok=True,parents=True)
rows=[];sizes=collections.Counter();best=None
for ix,(d,l,up,w,fl) in enumerate(recipes):
 source=preamble+body.replace('DECLARATION',declarations[d]).replace('LEADING',leading[l].replace('SKIP',skips[up])).replace('OUTPUT_DECL','' if 'dest=word' in declarations[d] else 'char *dest=word;').replace('WORDBODY',wordbody[w])
 tag=f'{ix:04d}-{d}-{l}-{up}-{w}-{fl}'
 cpp=T/(tag+'.cpp');obj=T/(tag+'.o');cpp.write_text(source)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags[fl],'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rows.append(dict(tag=tag,error=p.stderr[-140:]));continue
 try:r=m.run(obj,cfg)
 except ValueError as e:rows.append(dict(tag=tag,error=str(e)[:145]));continue
 row=dict(tag=tag,length=r['length'],match=r['exact_bytes'],relocations=r['relocations'],prefix=next(iter(r['different_offsets']),r['length']))
 rows.append(row);sizes[r['length']]+=1
 if row['length']==135 and row['relocations']==5 and (best is None or (row['match'],row['prefix'])>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(source);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(flags[fl])+'\n')
  if row['match']>23:print('IMPROVEMENT',json.dumps(row),flush=True)
  if row['match']==135:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rows),valid_original_length=sum(r.get('length')==135 and r.get('relocations')==5 for r in rows),lengths=dict(sizes),best=best,top=sorted((r for r in rows if 'match' in r),key=lambda x:(x['match'],x['prefix']),reverse=True)[:12]),indent=2))

#!/usr/bin/env python3
"""VUC GetNextWord original 135-byte scanner C++03 structural recovery.

Try authentic typed-byte nested delimiter dispatch and split control-flow
guard forms, informed by original signed-style JL/JG bounds and six delimiter
cases. Only original-size/three-relocation code is eligible as experimental;
overlap of wrong-sized candidates is diagnostic only.
"""
from pathlib import Path
import itertools,json,importlib.util,subprocess,collections
R=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
cfg=json.loads((R/'vuc/config/get_next_word_probe.json').read_text())
header='extern int currentPosition;\nextern int wordPosition;\nextern char word[1024];\n'
leads={
 'signed-bounds':'''for (;;) {
 unsigned char c = *input;
 if (c>10) { if(c !=32) break; }
 else if (c<9) break;
 ++position; ++input; currentPosition=position;
}''',
 'split-signed':'''for (;;) {
 const unsigned char c = *input;
 if (c <= 10) { if (c < 9) break; }
 else if (c != 32) break;
 ++position; ++input; currentPosition=position;
}''',
 'unsigned-paired':'''for (;;) {
 unsigned char c=*input;
 if(c==9 || c==10) {++position; ++input; currentPosition=position; continue;}
 if(c==32) {++position; ++input; currentPosition=position; continue;}
 break;
}''',
 'nested-switch':'''for (;;) {
 switch ((unsigned char)*input) {
 case 9:case 10:case 32: ++position; ++input; currentPosition=position; break;
 default:goto endleading;
 }
}
endleading:;''',
 'negative-goto':'''for (;;) {
 unsigned char c=*input;
 if (c>10) goto above10;
 if (c<9) goto endleading;
 goto whitespace;
above10:
 if(c!=32) goto endleading;
whitespace:
 ++position; ++input; currentPosition=position;
}
endleading:;'''
}
inner={
 'six-switch':'''unsigned char c=*input;
 switch(c) { case 0:case 9:case 32:case 44:case 46:case 91:goto endword; default:break; }''',
 'nested-case':'''unsigned char c=*input;
 if(c ==44)goto endword;
 if(c>44) { if(c==46||c==91)goto endword; }
 else {
  if(c==9)goto endword;
  if(c>9) {if(c==32)goto endword;} else {if(c==0)goto endword;}
 }''',
 'signed-guards':'''unsigned char c=*input;
 if(c ==44)goto endword;
 if(c>44) {if(c==46)goto endword; if(c==91)goto endword;}
 else {if(c==9)goto endword; if(c>9){if(c==32)goto endword;}else if(!c)goto endword;}''',
 'separated-switch':'''unsigned char c=*input;
 switch(c) { case 44:case 46:case 91:goto endword; }
 switch(c) { case 0:case 9:case 32:goto endword; }''',
 'comparison-chain':'''unsigned char c=*input;
 if(c==0||c==9||c==32||c==44||c==46||c==91)goto endword;''',
}
locals={
 'pos':'int position=currentPosition;\n',
 'register-pos':'register int position=currentPosition;\n',
 'first-input':'const char * const first=input; int position=currentPosition; (void)first;\n',
 'reg-input':'register const char * const scan=input;int position=currentPosition;(void)scan;\n',
}
flags={
 'j1':['-O2','-frename-registers','-falign-jumps=1'],
 'j4':['-O2','-frename-registers','-falign-jumps=4'],
 'j8':['-O2','-frename-registers','-falign-jumps=8'],
 'j16':['-O2','-frename-registers','-falign-jumps=16'],
 'plain':['-O2'],
}
T=Path('/tmp/vuc-binutils215/get-next-word-authentic-byte-guards-oct15e');T.mkdir(parents=True,exist_ok=True)
raw,elf=m.decomp.load();sec=next(x for x in elf['sections'] if x['name']=='.text');a=int(cfg['address'],0);ref=raw[sec['offset']+a-sec['address']:sec['offset']+a-sec['address']+135]
rank=[];sizes=collections.Counter();best=None
for (ln,lead),(inn,word),(local,decl),(flag,opts) in itertools.product(leads.items(),inner.items(),locals.items(),flags.items()):
 name=f'{ln}-{inn}-{local}-{flag}'
 body=header+'''const char *GetNextWord(const char *input) {
'''+decl+lead+'''
 wordPosition=position;
 char *dest=word;
 for (;;) {
'''+word+'''
 *dest++=c;
 ++input;
 ++position;
 currentPosition=position;
 }
endword:
 *dest=0;
 return input;
}
'''
 cpp=T/(name+'.cpp');obj=T/(name+'.o');cpp.write_text(body)
 p=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(cpp),'-o',str(obj)],capture_output=True,text=True)
 if p.returncode:rank.append(dict(tag=name,error=p.stderr[-115:]));continue
 try:b,r=m.prepare_named_function(obj.read_bytes(),cfg['function'],cfg['relocations'],a)
 except ValueError as e:rank.append(dict(tag=name,error=str(e)[:155]));continue
 score=sum(x==y for x,y in zip(ref,b));px=next((i for i,(x,y) in enumerate(zip(ref,b)) if x!=y),min(len(ref),len(b)))
 row=dict(tag=name,length=len(b),match=score,prefix=px,relocations=r);rank.append(row);sizes[len(b)]+=1
 if len(b)==135 and r==3 and (best is None or (score,px)>(best['match'],best['prefix'])):
  best=row;(T/'best.cpp').write_text(body);(T/'best.o').write_bytes(obj.read_bytes());(T/'best.flags').write_text(' '.join(opts)+'\n')
  print('STRICT',json.dumps(row),flush=True)
  if score==135:print('EXACT',json.dumps(row),flush=True);break
(T/'results.json').write_text(json.dumps(rank,indent=2)+'\n')
print(json.dumps(dict(attempted=len(rank),sizes=dict(sizes),best=best,top=sorted((r for r in rank if 'match' in r),key=lambda x:(x['match'],x['prefix']),reverse=True)[:12]),indent=2))

#!/usr/bin/env python3
"""Natural local/parameter lifetime source controls for VUC word scanner.

Compare original-length instruction data only. The intent is to restore
original EAX position, ECX input and EBX output without register asm or
object editing; GCC C++03 register hints are tested as source hypotheses.
"""
from pathlib import Path
from itertools import product
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2];T=Path('/tmp/vuc-binutils215/getnextword-local-types-oct14');T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location('m',R/'vuc/scripts/match_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(Path('/tmp/vuc-binutils215/getnextword-switch-registers-oct14')/'if-bounds-range-pointer-post.cpp').read_text()
decl='int pos=currentPosition;'
assert decl in source
definitions={
'int':decl,
'register-int':'register int pos=currentPosition;',
'unsigned':'unsigned pos=currentPosition;',
'register-unsigned':'register unsigned pos=currentPosition;',
'long':'long pos=currentPosition;',
'register-long':'register long pos=currentPosition;',
'const-reference-init':'int pos=(int&)currentPosition;',
}
prefixes={
'plain':"const char *GetNextWord(const char *input)",
'register-arg':"const char *GetNextWord(register const char *input)",
}
changes={
'baseline':('++pos;++input;currentPosition=pos;','++input; ++pos; currentPosition=pos;'),
'position-first':('++pos;++input;currentPosition=pos;','++pos; ++input; currentPosition=pos;'),
'assign-increment':('++pos;++input;currentPosition=pos;','++input; currentPosition=++pos;'),
}
ends={
'original':"*dest=0;\n    return input;",
'return-local':"const char *result = input;\n    *dest=0;\n    return result;",
'end-index':'*dest++=0;\n    return input;',
}
rank=[]
for ((d,declare),(name,signature),(u,rewrite),(e,finish),ren) in product(definitions.items(),prefixes.items(),changes.items(),ends.items(),(True,False)):
 candidate=source.replace(decl,declare).replace(prefixes['plain'],signature)
 candidate=candidate.replace(changes['baseline'][0],rewrite[1])
 candidate=candidate.replace(ends['original'],finish)
 key=f'{d}-{name}-{u}-{e}-'+('ren' if ren else 'plain')
 p=T/(key+'.cpp');p.write_text(candidate)
 flags=['-O2']+(['-frename-registers'] if ren else [])+['-falign-jumps=1']
 o=T/(key+'.o')
 c=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*flags,'-c',str(p),'-o',str(o)],capture_output=True,text=True)
 if c.returncode:
  print('COMPILE',key,c.stderr[-80:],flush=True);continue
 row=next((x for x in subprocess.check_output(['nm','-S',str(o)],text=True).splitlines() if x.split()[-1:]==['_Z11GetNextWordPKc']),'')
 size=int(row.split()[1],16) if row else -1
 if size==135:
  try:
   r=m.run(o,R/'vuc/config/get_next_word_probe.json')
   rank.append((r['exact_bytes'],size,key,r['different_offsets'][:12]))
   if r['exact']:print('NEW_EXACT',p,flush=True);raise SystemExit(0)
  except ValueError as ex:print('REJECT',key,str(ex)[:105],flush=True)
 else:rank.append((0,size,key))
print('ORIGINAL_LENGTH_COUNT',sum(x[1]==135 for x in rank))
print('BEST')
for row in sorted(rank,reverse=True)[:28]:print(row)

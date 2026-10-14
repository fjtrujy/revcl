#!/usr/bin/env python3
"""Recover VUC original byte-tested GetNextWord from switch dispatch and locals.

Original x86 dispatch compares 8-bit input in DL, owns a position local in
EAX, a scanner pointer in ECX and destination in EBX. Test C++03 source
spelling of semantic delimiter decisions with exact original-data relocations.
"""
from pathlib import Path
import itertools,importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/getnextword-switch-registers-oct14");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py");m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
header="extern int currentPosition;\nextern int wordPosition;\nextern char word[1024];\n"
lead={
 "switch":"""while (1) {
        unsigned char c = *input;
        switch (c) {
        case '\t':case '\n':case ' ': ++pos; ++input; currentPosition=pos;break;
        default: goto initial_done;
        }
    }
initial_done:""",
 "if-bounds":"""for (;;) {
        unsigned char c=*input;
        if (c>10) {if(c!=32) break;}
        else if (c<9) break;
        ++pos;++input;currentPosition=pos;
    }""",
 "if-simple":"""for (;;) {
        unsigned char c=*input;
        if (c!=9 && c!=10 && c!=32)break;
        ++pos;++input;currentPosition=pos;
    }""",
}
inner={
 "switch":"""switch(c) {
            case 0:case 9:case 32:case 44:case 46:case 91:goto end;
            default:break;
        }""",
 "range":"""if (c == 44) goto end;
        if (c > 44) {if (c == 46 || c == 91) goto end;}
        else {if (c == 9 || c == 0 || c == 32) goto end;}""",
 "range2":"""if(c==44)goto end;
        if(c>44) {
            if(c==46||c==91)goto end;
        } else {
            if(c==9 || c==0)goto end;
            if(c==32)goto end;
        }""",
 "compare-predicate":"""if (c==0 || c==9 || c==32 || c==44 || c==46 || c==91) goto end;""",
 "switch-typed":"""switch((unsigned char)c) {
            case '\0': case '\t': case ' ': case ',': case '.': case '[':
                goto end;
        }""",
}
upd={
 "position-first":"*dest++=c; ++pos; ++input; currentPosition=pos;",
 "pointer-first":"*dest++=c; ++input; ++pos; currentPosition=pos;",
 "post-increment":"*dest=c; ++pos; ++input; currentPosition=pos; ++dest;",
 "pointer-post":"*dest=c; ++input; ++pos; currentPosition=pos; ++dest;",
}
flags={
 "rj4":["-O2","-frename-registers","-falign-jumps=4"],
 "rj1":["-O2","-frename-registers","-falign-jumps=1"],
 "pj4":["-O2","-falign-jumps=4"],
 "pj1":["-O2","-falign-jumps=1"],
}
rank=[]
for l,g,u in itertools.product(lead,inner,upd):
 name=l+"-"+g+"-"+u
 source=header+"""
const char *GetNextWord(const char *input) {
    int pos=currentPosition;
    """+lead[l]+"""
    wordPosition=pos;
    char *dest=word;
    for (;;) {
        unsigned char c=*input;
        """+inner[g]+"""
        """+upd[u]+"""
    }
end:
    *dest=0;
    return input;
}
"""
 p=T/(name+".cpp");p.write_text(source)
 for f,opts in flags.items():
  obj=T/(name+"-"+f+".o")
  q=subprocess.run(['/tmp/vuc-binutils215/g++-jan25-as215',*opts,'-c',str(p),'-o',str(obj)],capture_output=True,text=True)
  if q.returncode:print('ERROR',name,f,q.stderr[-115:],flush=True);continue
  s=subprocess.check_output(['nm','-S',str(obj)],text=True)
  record=next((x for x in s.splitlines() if x.split()[-1:]==['_Z11GetNextWordPKc']),"")
  size=int(record.split()[1],16) if record else -1
  if size==135:
   try:
    res=m.run(obj,R/'vuc/config/get_next_word_probe.json')
    rank.append((res['exact_bytes'],size,name,f,res['different_offsets'][:8]))
    if res['exact']:print('NEW_EXACT',p,f,flush=True);raise SystemExit(0)
   except ValueError as e:print('REJECT',name,f,str(e)[:95],flush=True)
  else:rank.append((0,size,name,f))
 print('DONE',l,g,u,flush=True)
print('TOP')
for s in sorted(rank,reverse=True)[:35]:print(s)

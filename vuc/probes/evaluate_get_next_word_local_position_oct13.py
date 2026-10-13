#!/usr/bin/env python3
"""Recover GetNextWord's local-position loop and genuine delimiter switch.

The original stores currentPosition only after advancing input; newline is
skipped initially but *not* terminated inside the token. Test genuine period
C++03 control-flow forms with original relocations, no machine-byte edits.
"""
from pathlib import Path
import subprocess, importlib.util, itertools
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/get-next-word-register-oct13")
T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m", R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
header="extern int currentPosition;\nextern int wordPosition;\nextern char word[1024];\n"
prefixes={
 "local-increments":"""int position=currentPosition;
    for(;;) {
       unsigned char c=*input;
       if((c>=9 && c<=10) || c==32) {
          ++position;
          ++input;
          currentPosition=position;
       } else break;
    }""",
 "local-inline":"""int position=currentPosition;
    while(*input=='\\t' || *input=='\\n' || *input==' ') {
       ++position;
       ++input;
       currentPosition=position;
    }""",
 "local-outer":"""int position=currentPosition;
    for(;;) {
       unsigned char c=*input;
       if(c>10) { if(c!=32) break; }
       else if(c<9) break;
       ++position;
       ++input;
       currentPosition=position;
    }""",
 "local-reverse":"""int position=currentPosition;
    while(1) {
       unsigned char c=*input;
       if(c<9 || (c>10 && c!=32)) break;
       ++input;
       currentPosition=++position;
    }""",
}
ends={
 "switch-joined":"""switch(c) {
     case 0: case 9: case 32: case 44: case 46: case 91:
       goto done;
     default: break;
    }""",
 "switch-return":"""switch(c) {
     case 0: case 9: case 32: case 44: case 46: case 91:
       *output=0; return input;
     default: break;
    }""",
 "ifs":"""if(c==0 || c==9 || c==32 || c==44 || c==46 || c==91) goto done;""",
 "charcases":"""switch((char)c) {
     case 0: case '\\t': case ' ': case ',': case '.': case '[':
       goto done;
    }""",
 "charcases-return":"""switch((char)c) {
     case 0: case '\\t': case ' ': case ',': case '.': case '[':
       *output=0;return input;
    }""",
 "nested-comparisons":"""if(c==44) goto done;
    if(c<44) {
       if(c==0 || c==9 || c==32)goto done;
    } else {
       if(c==46 || c==91)goto done;
    }""",
}
update={
 "in-body":"""*output++=c;
       ++input;
       currentPosition=++position;""",
 "post-body":"""*output++=c;
       ++input;
       ++position;
       currentPosition=position;""",
 "first-global":"""*output++=c;
       ++position;
       ++input;
       currentPosition=position;"""
}
flags={
 "rj1":["-O2","-frename-registers","-falign-jumps=1"],
 "rj4":["-O2","-frename-registers","-falign-jumps=4"],
 "plain-j1":["-O2","-falign-jumps=1"],
 "plain-j4":["-O2","-falign-jumps=4"],
}
rank=[]
for pre, pbody in prefixes.items():
 for suffix, guard in ends.items():
  for up, ubody in update.items():
   code=header+f"""
const char *GetNextWord(const char *input) {{
    {pbody}
    wordPosition=position;
    char *output=word;
    for(;;) {{
       unsigned char c=*input;
       {guard}
       {ubody}
    }}
done:
    *output=0;
    return input;
}}
"""
   src=T/(pre+"-"+suffix+"-"+up+".cpp");src.write_text(code)
   for tag,opts in flags.items():
    obj=T/(src.stem+"-"+tag+".o")
    cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
    if cp.returncode:
     print("ERROR",src.stem,tag,cp.stderr[-90:],flush=True);continue
    output=subprocess.check_output(["nm","-S",str(obj)],text=True)
    sym=next((line for line in output.splitlines() if line.endswith("_Z11GetNextWordPKc")),"")
    size=int(sym.split()[1],16) if sym else -1
    if size==135:
     try:
      r=m.run(obj,R/"vuc/config/get_next_word_probe.json")
      rank.append((r["exact_bytes"],size,src.stem,tag,r["different_offsets"][:10]))
      if r["exact"]:
       print("EXACT",src,tag,flush=True);raise SystemExit(0)
     except ValueError as e:rank.append((0,size,src.stem,tag,str(e)[:90]))
    else:rank.append((0,size,src.stem,tag,"length"))
  print("DONE",pre,suffix,flush=True)
print("TOP")
for row in sorted(rank,reverse=True)[:30]:print(row)

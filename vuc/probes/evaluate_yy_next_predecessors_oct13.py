#!/usr/bin/env python3
"""Restore real lexical Flex predecessor definitions before buffer growth.

Original VUC function order is LexerInput(), LexerOutput(), yy_get_next_buffer().
Tests actual recovered method bodies, no dummy/invented methods, while keeping
original 550-byte target + genuine literals/relocations as mandatory gates.
"""
from pathlib import Path
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/yy-next-authentic-predecessors-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
s=(R/"vuc/probes/yy_get_next_buffer_partial_oct13.cpp").read_text()
cut=s.index("int yyFlexLexer::yy_get_next_buffer()")
init=s[:cut]
body=s[cut:]
lexerinput='''int yyFlexLexer::LexerInput(char *buffer, int maxSize)
{
    if (yyin->eof() || yyin->fail()) return 0;
    yyin->get(buffer[0]);
    if (yyin->eof()) return 0;
    return yyin->bad() ? -1 : 1;
}
'''
lexeroutput='''void yyFlexLexer::LexerOutput(const char *buffer, int size)
{
    yyout->write(buffer, size);
}
'''
cases={
"isolated":"",
"real-output":lexeroutput,
"real-input":lexerinput,
"real-input-output":lexerinput+lexeroutput,
"real-output-input":lexeroutput+lexerinput,
}
opts={
"j1":["-O2","-frename-registers","-falign-jumps=1"],
"j2":["-O2","-frename-registers","-falign-jumps=2"],
"j4":["-O2","-frename-registers","-falign-jumps=4"],
"none":["-O2","-frename-registers"],
}
results=[]
for name,prefix in cases.items():
 p=T/(name+".cpp");p.write_text(init+prefix+body)
 for f,flags in opts.items():
  obj=T/(name+"-"+f+".o")
  comp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if comp.returncode:
   print("ERROR",name,f,comp.stderr[-135:],flush=True);continue
  nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
  row=next((x for x in nm.splitlines() if x.endswith("_ZN11yyFlexLexer18yy_get_next_bufferEv")),"")
  size=int(row.split()[1],16) if row else -1
  if size != 550:
   results.append((0,size,name,f));continue
  try:
   r=m.run(obj,R/"vuc/config/yy_get_next_buffer_oct13_probe.json")
   results.append((r["exact_bytes"],size,name,f))
   if r["exact"]:print("EXACT",p,f,flush=True);raise SystemExit(0)
  except ValueError as e:print("REJECT",name,f,str(e)[:115],flush=True)
 print("DONE",name,flush=True)
print("BEST")
for result in sorted(results,reverse=True)[:25]:print(result)

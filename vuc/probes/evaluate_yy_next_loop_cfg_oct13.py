#!/usr/bin/env python3
"""Control-flow recovery of genuine Flex 2004 buffer-growth loop.

Compare natural while/for/do and loop-exit shapes to original 550-byte x86
function. All variant outputs remain under /tmp; relocations and original
error strings must pass the original-ELF matcher.
"""
from pathlib import Path
import subprocess, importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/yy-next-growth-loop-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=(R/"vuc/probes/yy_get_next_buffer_partial_oct13.cpp").read_text()
start=source.index("        while (num_to_read <= 0) {")
end=source.index("        if (num_to_read > 8192)",start)
part=source[start:end]
assert part.strip().endswith("}")
inner=part[part.index("{")+1:part.rindex("}")]
forms={
 "while":part,
 "for":"        for (; num_to_read <= 0; ) {"+inner+"        }\n",
 "negated-while":"        while (!(num_to_read > 0)) {"+inner+"        }\n",
 "while-if-break":"        while (1) {\n            if (num_to_read > 0) break;"+inner+"        }\n",
 "for-if-break":"        for (;;) {\n            if (num_to_read > 0) break;"+inner+"        }\n",
 "if-do":"        if (num_to_read <= 0) do {"+inner+"        } while (num_to_read <= 0);\n",
 "if-for":"        if (num_to_read <= 0) for (;;) {"+inner+"            if (num_to_read > 0) break;\n        }\n",
 "do-early-continue":"        if (num_to_read <= 0) {\n            do {"+inner+"            } while (num_to_read <= 0);\n        }\n",
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j1-loop1":["-O2","-frename-registers","-falign-jumps=1","-falign-loops=1"],
 "j1-loop16":["-O2","-frename-registers","-falign-jumps=1","-falign-loops=16"]
}
rank=[]
for label,fragment in forms.items():
 p=T/(label+".cpp");p.write_text(source[:start]+fragment+source[end:])
 for setting,opts in flags.items():
  obj=T/(label+"-"+setting+".o")
  cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cc.returncode:print("ERROR",label,setting,cc.stderr[-125:],flush=True);continue
  nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
  sym=next((x for x in nm.splitlines() if x.endswith("_ZN11yyFlexLexer18yy_get_next_bufferEv")),"")
  size=int(sym.split()[1],16) if sym else -1
  if size==550:
   try:
    result=m.run(obj,R/"vuc/config/yy_get_next_buffer_oct13_probe.json")
    rank.append((result["exact_bytes"],size,label,setting))
    if result["exact"]:
     print("NEW_EXACT",p,setting,flush=True);raise SystemExit(0)
   except ValueError as e:print("REJECT",label,setting,str(e)[:115],flush=True)
  else:rank.append((0,size,label,setting))
 print("DONE",label,flush=True)
print("BEST")
for row in sorted(rank,reverse=True)[:28]:print(row)

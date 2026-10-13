#!/usr/bin/env python3
"""Resolve Value::TypeConvert 184/220 residual around the common false return.

Authenticated original copies 32-byte operands verbatim and uses EAX for
the default false result; an early nested-switch hypothesis used ECX and
an extra move, shifting the rest of the function by two bytes. Test only
period-normal break/return structure in actual type conversion logic.
"""
from pathlib import Path
import importlib.util,subprocess,itertools
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/value-type-convert-false-exits-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
original=(R/"vuc/probes/value_type_convert_oct13.cpp").read_text()
prefix=original[:original.index("    if (left.type == 1")]
cases={
 "group3":"""    case 3:
        switch (right.type) {
        case 1: case 2:
            right.values[0] = (float)right.bits;
            right.type = 3;
            return true;
        }
        break;""",
 "group12":"""    case 1: case 2:
        switch (right.type) {
        case 1: case 2: right.type = 1; return true;
        case 3:
            left.values[0] = (float)left.bits;
            left.type = 3;
            return true;
        }
        break;""",
}
forms={}
for order in itertools.permutations(("group3","group12")):
 for default in ("none","explicit-break","return-false"):
  cases_src="\n".join(cases[n] for n in order)
  if default=="explicit-break":cases_src+="\n    default: break;"
  if default=="return-false":cases_src+="\n    default: return false;"
  body="    switch (left.type) {\n"+cases_src+"\n    }\n    return false;"
  forms["-".join(order)+"-"+default]=body
# Original-looking switch with a shared return, but shortcutting
# invalid inner values by jumping to the final return using a block bool.
for suffix in ("shared","per-outer"):
 form="""    switch (left.type) {
    case 3:
        if (right.type == 1 || right.type == 2) {
            right.values[0] = (float)right.bits;
            right.type = 3;
            return true;
        }
        %s
    case 1: case 2:
        if (right.type == 1 || right.type == 2) {
            right.type = 1;
            return true;
        }
        if (right.type == 3) {
            left.values[0] = (float)left.bits;
            left.type = 3;
            return true;
        }
        break;
    }
    return false;""" % ("break;" if suffix=="shared" else "return false;")
 forms["if-"+suffix]=form
opts={
 "rj1":["-O2","-frename-registers","-falign-jumps=1"],
 "rj2":["-O2","-frename-registers","-falign-jumps=2"],
 "rj4":["-O2","-frename-registers","-falign-jumps=4"],
 "r":["-O2","-frename-registers"],
}
rank=[]
for label,body in forms.items():
 p=T/(label+".cpp")
 p.write_text(prefix+body+"\n}\n")
 for tag,flags in opts.items():
  o=T/(label+"-"+tag+".o")
  c=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(p),"-o",str(o)],capture_output=True,text=True)
  if c.returncode:print("ERROR",label,tag,c.stderr[-130:],flush=True);continue
  nm=subprocess.check_output(["nm","-S",str(o)],text=True)
  sym=next((s for s in nm.splitlines() if s.endswith("_ZN5Value11TypeConvertERKS_S1_RS_S2_")),"")
  size=int(sym.split()[1],16) if sym else -1
  if size==220:
   try:
    match=m.run(o,R/"vuc/config/value_type_convert_oct13_probe.json")
    rank.append((match["exact_bytes"],label,tag,size,match["different_offsets"][:14]))
    if match["exact"]:print("NEW_EXACT",label,tag,p,flush=True)
   except ValueError as e:rank.append((0,label,tag,size,str(e)[:80]))
  else:rank.append((0,label,tag,size,[]))
 print("checked",label,flush=True)
print("TOP")
for item in sorted(rank,key=lambda x:x[0],reverse=True)[:35]:print(item)

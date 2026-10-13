#!/usr/bin/env python3
"""Check TypeConvertForce against genuine TypeConvert predecessor and alignment.

The original VUC source defines TypeConvert before TypeConvertForce.
Register-renamer and jump-alignment changes are measured against the
complete 257-byte original body with no custom machine-byte edits.
"""
from pathlib import Path
import subprocess,importlib.util,itertools
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/typeconvertforce-authentic-context-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
candidate=(Path("/tmp/vuc-binutils215/typeconvertforce-control-oct13")/"baseline-switch-reverse.cpp").read_text()
real=(R/"vuc/src/ValueTypeConvert.cpp").read_text()
realdef=real[real.index("bool Value::TypeConvert("):]
candidate=candidate.replace("    static bool TypeConvertForce(", "    static bool TypeConvert(const Value&,const Value&,Value&,Value&);\n    static bool TypeConvertForce(")
split=candidate.index("bool Value::TypeConvertForce(")
for context in ("isolated","original-predecessor"):
 source=candidate if context=="isolated" else candidate[:split]+realdef+"\n"+candidate[split:]
 src=T/(context+".cpp");src.write_text(source)
 for rename in (False,True):
  for jumps,labels,loops in itertools.product(("1","2","4"),("1","2","4"),("1","2","4")):
   args=["-O2"]+(["-frename-registers"] if rename else [])+["-falign-jumps="+jumps,"-falign-labels="+labels,"-falign-loops="+loops]
   key=context+("-ren" if rename else "-plain")+"-j"+jumps+"-l"+labels+"-loop"+loops
   obj=T/(key+".o")
   cc=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*args,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
   if cc.returncode:
    print("ERROR",key,cc.stderr[-180:],flush=True);continue
   size=next((line.split()[1] for line in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines() if line.endswith("_ZN5Value16TypeConvertForceE6TypeIDRKS_S2_RS_S3_")),None)
   if not size or int(size,16)!=257:continue
   try:
    result=m.run(obj,R/"vuc/config/value_type_convert_force_oct13_probe.json")
    print("ORIGINAL_SIZE",key,result["exact_bytes"],result["different_offsets"][:15],flush=True)
    if result["exact"]:print("NEW_EXACT",str(src),args,flush=True);raise SystemExit(0)
   except ValueError as e:print("REJECT",key,str(e)[:100],flush=True)
 print("DONE",context,flush=True)

#!/usr/bin/env python3
"""Source declaration and initialization-order probes for VUC currFileName TU."""
from pathlib import Path
import subprocess,importlib.util
ROOT=Path(__file__).resolve().parents[2]
TEMP=Path("/tmp/vuc-binutils215")
sp=importlib.util.spec_from_file_location("vuc_match",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
base=(ROOT/"vuc/src/CurrFileInitialization.cpp").read_text()
decls={
 "char":"char *currFileName = (char *)\"\";",
 "const-char":"const char *currFileName = \"\";",
 "void":"void *currFileName = (void *)\"\";",
 "volatile-char":"char *volatile currFileName = (char *)\"\";",
 "volatile-const":"const char *volatile currFileName = \"\";",
 "char-array":"char currFileName[] = \"\";",
}
errors={
 "bool":"bool error;",
 "char":"char error;",
 "uchar":"unsigned char error;",
}
flags={
 "ren":["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"],
 "ren-j1":["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections","-falign-jumps=1"],
 "no-ren":["-O2","-fuse-cxa-atexit","-ffunction-sections"]
}
scores=[]
for dname,decl in decls.items():
 for ename,err in errors.items():
  source=base.replace('char *currFileName = (char *)"";',decl).replace("bool error;",err)
  p=TEMP/("currfile-"+dname+"-"+ename+".cpp");p.write_text(source)
  for opt,fl in flags.items():
   obj=TEMP/("currfile-"+dname+"-"+ename+"-"+opt+".o")
   cp=subprocess.run([str(TEMP/"g++-jan25-as215"),*fl,"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
   if cp.returncode:
    scores.append((0,0,dname,ename,opt,cp.stderr[-120:]));continue
   results=[]
   for name in ("iostream_global_curr_file_probe","iostream_static_curr_file_probe"):
    try:
     r=m.run(obj,ROOT/("vuc/config/"+name+".json"))
     results.append(r["exact_bytes"])
    except Exception:
     results.append(0)
   scores.append((results[0],results[1],dname,ename,opt,""))
scores.sort(reverse=True)
for row in scores[:26]:print(*row,flush=True)

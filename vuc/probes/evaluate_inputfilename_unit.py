#!/usr/bin/env python3
"""Search plausible VUC inputFileName compilation-unit context for exact wrapper."""
from pathlib import Path
import subprocess, json
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215")
gcc=T/"g++-jan25-as215"
base='#include <iostream>\nchar inputFileName[256] = {};\n'
units={
 "bare":base,
 "line_methods":base+(R/"vuc/src/ListFile.cpp").read_text(),
 "line_print":base+(R/"vuc/src/ListFilePrintLine.cpp").read_text(),
 "algorithm":base+"int file_count(int n) { return n>0?n:0; }\n",
 "instance":base+"static std::ios_base::Init extra_initializer;\n",
 "files_header":'#include <vector>\n#include <string>\n'+base,
 "namespace":base+"namespace nested { int a = 2; }\n",
 "inline_static":base+"static int print_line_index(int x) { return x + 1; }\n",
}
opts=[
("-O2-ren",["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"]),
("-O2-j1",["-O2","-falign-jumps=1","-fuse-cxa-atexit","-ffunction-sections"]),
("-O2-ren-j1",["-O2","-frename-registers","-falign-jumps=1","-fuse-cxa-atexit","-ffunction-sections"]),
]
allrows=[]
for kind,body in units.items():
 p=T/("inputfilename-context-"+kind+".cpp");p.write_text(body)
 for suffix,flags in opts:
  obj=T/("inputfilename-context-"+kind+suffix+".o")
  cp=subprocess.run([str(gcc),*flags,"-I",str(R/"vuc/src"),"-c",str(p),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode:
   allrows.append((0,kind,suffix,cp.stderr[-180:]));continue
  pr=subprocess.run(["python3",str(R/"vuc/scripts/match_probe.py"),"--object",str(obj),"--config",str(R/"vuc/config/iostream_global_inputfilename_oct12_probe.json")],capture_output=True,text=True)
  try:
   d=json.loads(pr.stdout)
   allrows.append((d["exact_bytes"],kind,suffix,"length="+str(d["length"])))
  except:allrows.append((0,kind,suffix,pr.stderr[-150:]))
for r in sorted(allrows,reverse=True):print(*r)

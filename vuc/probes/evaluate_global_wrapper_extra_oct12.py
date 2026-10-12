#!/usr/bin/env python3
"""Extra historical GCC global-wrapper flags, preserving exact-only comparisons."""
import subprocess,importlib.util,json
from pathlib import Path
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/global-wrapper-extra-oct12")
T.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location("matcher",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
names=("matchStack","exportModule","script_debug","curr_file")
variants=(
 ("basic",[]),("nozero",["-fno-zero-initialized-in-bss"]),
 ("nocommon",["-fno-common"]),("nozero-nocommon",["-fno-zero-initialized-in-bss","-fno-common"]),
 ("noschedule",["-fno-schedule-insns2"]),
 ("nozero-noschedule",["-fno-zero-initialized-in-bss","-fno-schedule-insns2"]),
 ("jump1",["-falign-jumps=1"]),("jump4",["-falign-jumps=4"]),
 ("nozero-jump1",["-fno-zero-initialized-in-bss","-falign-jumps=1"]),
 ("nozero-jump4",["-fno-zero-initialized-in-bss","-falign-jumps=4"]),
 ("nopeephole",["-fno-peephole2"]),("nozero-nopeephole",["-fno-zero-initialized-in-bss","-fno-peephole2"]),
)
for name in names:
 cfg=R/("vuc/config/iostream_global_"+name+"_probe.json")
 config=json.loads(cfg.read_text())
 src=R/config["source"]
 rows=[]
 for base in ("-O2","-Os"):
  for rename in (True,False):
   for id,extra in variants:
    flags=[base,"-fuse-cxa-atexit","-ffunction-sections",*(["-frename-registers"] if rename else []),*extra]
    obj=T/"candidate.o"
    cmd=["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(src),"-o",str(obj)]
    p=subprocess.run(cmd,capture_output=True,text=True)
    if p.returncode:continue
    try:
     result=m.run(obj,cfg)
     rows.append((result["exact_bytes"],result["length"],base,rename,id,flags))
     if result["exact"]:
      print("EXACT",name,flags,flush=True)
      raise SystemExit(0)
    except ValueError:
     rows.append((0,0,base,rename,id,flags))
 print(name,"checked",len(rows),"best",sorted(rows,key=lambda a:a[0],reverse=True)[:4],flush=True)

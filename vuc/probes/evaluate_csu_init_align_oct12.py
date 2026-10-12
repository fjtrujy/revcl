#!/usr/bin/env python3
"""Verify ordinary C source and compiler alignment variants for __libc_csu_init."""
import importlib.util
from itertools import product
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/csu-sweep-oct12")
T.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location("matcher",ROOT/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
CFG=ROOT/"vuc/config/libc_csu_init_pic_probe.json"
SRC=ROOT/"vuc/src/LibcCsu.c"
results=[]
compilers=[("jan","/tmp/vuc-gcc333-jan25/build/gcc"),
           ("dec","/tmp/vuc-gcc333-dec06/build/gcc")]
opts=[[],["-frename-registers"]]
loops=["1","2","4","8","16","32"]
jumps=["1","2","4","8","16"]
labels=["1","2","4"]
i=0
for (name,compiler),extra,loop,jump,label in product(compilers,opts,loops,jumps,labels):
    flags=["-O2","-fPIC",*extra,"-falign-loops="+loop,"-falign-jumps="+jump,"-falign-labels="+label]
    obj=T/"candidate.o"
    cmd=[compiler+"/xgcc","-B/tmp/vuc-binutils215/tool-bin/","-B"+compiler+"/","-x","c",*flags,"-Wa,--32","-c",str(SRC),"-o",str(obj)]
    proc=subprocess.run(cmd,capture_output=True,text=True)
    if proc.returncode: continue
    try:
        match=m.run(obj,CFG)
        record=(match["exact_bytes"],match["length"],name,extra,loop,jump,label)
        results.append(record)
        if match["exact"]:
            print("EXACT",record,flush=True)
            raise SystemExit(0)
    except ValueError as e:
        results.append((0,0,name,extra,loop,jump,label))
    i+=1
    if i%40==0:print("Checked",i,"best",sorted(results,reverse=True)[:2],flush=True)
for row in sorted(results,reverse=True)[:20]:print(*row,flush=True)

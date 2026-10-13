#!/usr/bin/env python3
"""Locate the original Value::ToString's asymmetric branch alignment.

The original code has an unaligned case-4 target at +0x83 and an
aligned case-3 target at +0xc0. The first context probe matches 159/221
with a one-byte case-4 displacement and otherwise authentic literals.
Vary only historical GCC 3.3 branch/label/loop alignment settings.
"""
from pathlib import Path
from itertools import product
import importlib.util,subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/tostring-alignment-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=R/"vuc/probes/ValueToStringUnit.cpp"
cfg=R/"vuc/config/value_to_string_oct13_probe.json"
rank=[]
for jumps,labels,loops in product(("1","2","3","4","8","16"),("1","2","3","4","8","16"),("1","4","8","16")):
 opts=["-O2","-frename-registers","-falign-jumps="+jumps,"-falign-labels="+labels,"-falign-loops="+loops]
 key="j"+jumps+"-b"+labels+"-l"+loops
 obj=T/(key+".o")
 cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(source),"-o",str(obj)],capture_output=True,text=True)
 if cp.returncode:
  print("COMPILE",key,cp.stderr[-100:],flush=True);continue
 result=None
 try:
  result=m.run(obj,cfg)
  rank.append((result["exact_bytes"],result["length"],key))
  if result["exact"]:
   print("NEW_EXACT",key,source,flush=True)
   raise SystemExit(0)
 except ValueError as e:
  rank.append((0,-1,key))
print("TOP")
for row in sorted(rank,reverse=True)[:40]:print(row)

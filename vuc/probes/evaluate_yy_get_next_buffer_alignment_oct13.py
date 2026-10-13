#!/usr/bin/env python3
"""Bounded GCC 3.3.3 alignment controls for recovered Flex buffer-growth C++.

Check ONLY original-size (550-byte) functions with all three original strings,
their authenticated initialized bytes, and the original realloc relocation.
No synthetic alignment padding is inserted into C++.
"""
from pathlib import Path
from itertools import product
import subprocess,importlib.util
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/yy-next-layout-oct13");T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("match",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
src=R/"vuc/probes/yy_get_next_buffer_partial_oct13.cpp"
conf=R/"vuc/config/yy_get_next_buffer_oct13_probe.json"
scores=[]
for jumps,labels,loops in product((1,2,4,8),(1,2,4,8,16),(1,4,8)):
 flag=[ "-O2", "-frename-registers",f"-falign-jumps={jumps}",f"-falign-labels={labels}",f"-falign-loops={loops}"]
 key=f"j{jumps}-b{labels}-loop{loops}"
 obj=T/(key+".o")
 build=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flag,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
 if build.returncode:
  print("ERROR",key,build.stderr[-140:],flush=True);continue
 nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
 row=next((x for x in nm.splitlines() if x.endswith("_ZN11yyFlexLexer18yy_get_next_bufferEv")),"")
 size=int(row.split()[1],16) if row else -1
 if size!=550:
  scores.append((0,size,key));continue
 try:
  r=m.run(obj,conf)
  scores.append((r["exact_bytes"],size,key))
  if r["exact"]:
   print("NEW_EXACT",key,flag,flush=True)
   break
 except ValueError as e:print("REJECT",key,str(e)[:95],flush=True)
print("TOP")
for row in sorted(scores,reverse=True)[:25]:print(row)

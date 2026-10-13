#!/usr/bin/env python3
"""Measure AddLineComment's original 162-byte block-layout residual.

At -falign-jumps=4, the candidate inserts a two-byte NOP before the
line-duplicate branch at +0x82, shifting 14 bytes, while the original
has an EH landing-pad NOP at +0x8c. Vary historical alignment controls
on the real instruction-list source unit and score authentic relocations.
"""
from itertools import product
from pathlib import Path
import importlib.util, subprocess
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/linecomment-align-oct13");T.mkdir(parents=True,exist_ok=True)
sp=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
source=R/"vuc/probes/instruction_line_with_unit_oct13.cpp"
cfg=R/"vuc/config/instruction_add_line_comment_context_oct13_probe.json"
rows=[]
for jump,labels,loops,extra in product(("1","2","3","4","8"),("1","2","4"),("1","4"),("", "-ffunction-sections")):
  opts=["-O2","-frename-registers",f"-falign-jumps={jump}",f"-falign-labels={labels}",f"-falign-loops={loops}"]
  if extra:opts.append(extra)
  key=f"j{jump}-b{labels}-l{loops}"+("s" if extra else "")
  obj=T/(key+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(source),"-o",str(obj)],capture_output=True,text=True)
  if cp.returncode: print("COMPILE_FAIL",key,cp.stderr[-120:],flush=True);continue
  try:
   d=m.run(obj,cfg)
   rows.append((d["exact_bytes"],d["length"],key,d["different_offsets"][:18]))
   if d["exact"]:
    print("BYTE_EXACT",key,flush=True);raise SystemExit(0)
  except ValueError as e:
   rows.append((0,0,key,str(e)[:90]))
print("RANKED")
for row in sorted(rows,reverse=True)[:35]:print(row)

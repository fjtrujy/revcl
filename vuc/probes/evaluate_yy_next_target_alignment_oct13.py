#!/usr/bin/env python3
"""Test 16-byte GCC jump/loop/label alignment against the original Flex CFG.

Original yy_get_next_buffer has 16-byte entry at +0x50, copy loop +0x60,
and realloc loop +0x160. The existing 430/550 probe has entry +0x4b and
growth +0x15c. Change real GCC alignment flags only, never binary padding.
"""
from pathlib import Path
import subprocess,importlib.util,itertools
R=Path(__file__).resolve().parents[2]
T=Path("/tmp/vuc-binutils215/yy-next-target-alignment-oct13");T.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",R/"vuc/scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
src=R/"vuc/probes/yy_get_next_buffer_partial_oct13.cpp"
cfg=R/"vuc/config/yy_get_next_buffer_oct13_probe.json"
rank=[]
for j,l,k in itertools.product((1,4,8,16),(1,4,8,16),(1,4,8,16)):
 name=f"j{j}-label{l}-loop{k}"
 obj=T/(name+".o")
 flags=["-O2","-frename-registers",f"-falign-jumps={j}",f"-falign-labels={l}",f"-falign-loops={k}"]
 p=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
 if p.returncode:print("COMPILE_FAILED",name,p.stderr[-105:],flush=True);continue
 nm=subprocess.check_output(["nm","-S",str(obj)],text=True)
 row=next((x for x in nm.splitlines() if x.endswith("_ZN11yyFlexLexer18yy_get_next_bufferEv")),"")
 sz=int(row.split()[1],16) if row else -1
 if sz!=550:rank.append((0,sz,name));continue
 try:
  r=m.run(obj,cfg)
  rank.append((r["exact_bytes"],sz,name))
  if r["exact"]:
   print("NEW_EXACT",name,flush=True);raise SystemExit(0)
 except ValueError as e:print("REJECT",name,str(e)[:105],flush=True)
print("BEST")
for x in sorted(rank,reverse=True)[:25]:print(x)

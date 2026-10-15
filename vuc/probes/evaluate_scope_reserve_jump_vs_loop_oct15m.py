#!/usr/bin/env python3
"""Instruction-led GCC 3.3 alignment controls for original Scope::reserve.

Original 241-byte helper contains a 2-byte NOP at its cleanup loop entry
(+0x6e), while the initial branch target at +0x36 is unaligned. The
current 183/241 candidate reverses the NOP placement. Test historical
loop-vs-branch alignment options without changing STL implementation.
All authentic literal/relocation positions and the original length must
pass before byte scores can be reported.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

ROOT=Path(__file__).resolve().parents[2]
VUC=ROOT/"vuc"
OUT=Path("/tmp/vuc-binutils215/scope-reserve-alignment-oct15m")
OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("match",VUC/"scripts/match_probe.py")
match=importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
conf=VUC/"config/scope_vector_reserve_partial_oct15m.json"
cfg=json.loads(conf.read_text())
src=ROOT/cfg["source"]
assert "ScopeVectorReserveOriginalUnit" in src.name
fixed=["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"]
opts={
 "selected-j8": ["-falign-jumps=8"],
 "j1-loop8": ["-falign-jumps=1","-falign-loops=8"],
 "j1-loop16": ["-falign-jumps=1","-falign-loops=16"],
 "j2-loop8": ["-falign-jumps=2","-falign-loops=8"],
 "j4-loop8": ["-falign-jumps=4","-falign-loops=8"],
 "j8-loop8": ["-falign-jumps=8","-falign-loops=8"],
 "j8-loop16": ["-falign-jumps=8","-falign-loops=16"],
}
rows=[]
for snap in ("jan25","dec06"):
 for tag,flags in opts.items():
  obj=OUT/(snap+"-"+tag+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",*fixed,*flags,"-c",str(src),"-o",str(obj)],capture_output=True,text=True)
  row={"snapshot":snap,"source":"real ScopeVectorLifecycle.cpp","flags":tag}
  if cp.returncode:
   row["error"]=cp.stderr[-160:]
  else:
   try:
    s=match.run(obj,conf)
    row.update(length=s["length"],matched=s["exact_bytes"],
               relocations=s["relocations"],literals=s["verified_literal_bytes"],
               differing=s["different_offsets"][:72])
   except ValueError as e:
    row["error"]=str(e)
  print(tag,snap,row.get("length"),row.get("matched"),row.get("error",""),flush=True)
  rows.append(row)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

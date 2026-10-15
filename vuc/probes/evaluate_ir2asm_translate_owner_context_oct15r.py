#!/usr/bin/env python3
"""Recover original IR2Asm and Translate source-unit predecessor context.

Original ELF immediately precedes BroadcastMove with the 66-byte
iostream static-initialization unit, and Translate(Type) with the same
66-byte startup. Compile their actual period-C++03 source plus genuine
existing accepted preamble; compare byte/relocation/literals strictly.
Generated .o and candidates remain in the historical Lima guest.
"""
from pathlib import Path
import subprocess, json, importlib.util
R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-binutils215/ir2asm-translate-owner-oct15r")
O.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
config = {
  "broadcast":R/"config/broadcast_move_partial_oct15q.json",
  "translate":R/"config/translate_type_partial_oct15q.json",
}
source={
 "broadcast":(R/"probes/BroadcastMoveCommonAddOct15q.cpp").read_text(),
 "translate":(R/"probes/TranslateTypeFullOct15q.cpp").read_text(),
}
realunit={
 "broadcast":(R/"src/IR2AsmBroadcastUnit.cpp").read_text(),
 "translate":(R/"src/TranslateTypeUnit.cpp").read_text()
}
def absheader(s):
 return s.replace('#include "../src/MLGameString.h"', '#include "'+str(R/"src/MLGameString.h")+'"').replace('#include "MLGameString.h"','#include "'+str(R/"src/MLGameString.h")+'"')
# TranslateTypeUnit uses a provisional fake class hierarchy incompatible
# with original five Type_Data types; retain only its genuine iostream TU
# predecessor, not its inaccurate application implementation.
units={}
for name in ("broadcast","translate"):
 base=absheader(source[name])
 units[(name,"baseline")]=base
 units[(name,"iostream-predecessor")]='#include <iostream>\n'+base
 if name=="broadcast":
  orig=absheader(realunit[name])
  body=base[base.index("IR::Register BroadcastMove("):]
  old=orig[:orig.index("IR::Register BroadcastMove(")]
  units[(name,"genuine-accepted-unit")]=old+body
  units[(name,"iostream-and-genuine")]= '#include <iostream>\n'+old+body
 if name=="translate":
  orig=absheader(realunit[name])
  units[(name,"original-iostream-preamble")]=orig[:orig.index('struct Type {')]+base

rows=[]
for (name,recipe),code in units.items():
 path=O/(name+"-"+recipe+".cpp")
 path.write_text(code)
 for snap in ("jan25","dec06"):
  for suffix,flags in {
    "selected":json.loads(config[name].read_text())["flags"],
    "real-unit": ["-O2","-frename-registers","-fuse-cxa-atexit","-ffunction-sections"],
  }.items():
   obj=O/(name+"-"+recipe+"-"+snap+"-"+suffix+".o")
   p=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",*flags,"-c",str(path),"-o",str(obj)],capture_output=True,text=True)
   row={"target":name,"recipe":recipe,"snapshot":snap,"flags":suffix}
   if p.returncode: row["error"]=p.stderr[-230:]
   else:
    try:
     score=m.run(obj,config[name])
     row.update(length=score["length"],matched=score["exact_bytes"],relocations=score["relocations"],literals=score["verified_literal_bytes"],differences=score["different_offsets"][:30])
    except ValueError as ex:row["error"]=str(ex)
   rows.append(row)
   print({k:v for k,v in row.items() if k!="differences"},flush=True)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

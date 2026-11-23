#!/usr/bin/env python3
"""Combined historical GCC jump/label alignment for Scope-copy EH boundary.

The original 149-byte STL helper has four natural compiler alignment bytes
before the exception handler. Earlier one-dimensional jump/label controls
failed. This tests coupled alignment settings without modifying source code,
inserting padding, patching binaries, or altering relocation assumptions.
"""
from pathlib import Path
import json,subprocess,importlib.util
V=Path(__file__).resolve().parents[1]; O=Path("/tmp/vuc-scope-combined-alignment-20261123");O.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
cfg=json.loads((V/"config/hash_scope_uninitialized_probe.json").read_text())
cfg["function"]=("_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iterator"
"IP11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS3_ELi513EE"
"St6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type")
cfg["address"]="0x0806a840";cfg["size"]=149;cfg["source"]="vuc/src/SymbolTableUnit.cpp"
newcfg=O/"original-helper.json";newcfg.write_text(json.dumps(cfg,indent=2)+"\n")
rows=[]
for jumps in range(1,9):
 for labels in range(1,9):
  for snap in ("jan25","dec06"):
   label=f"j{jumps}-l{labels}-{snap}"
   obj=O/(label+".o")
   cmd=["/tmp/vuc-binutils215/g++-"+snap+"-as215",
        "-O2","-frename-registers",f"-falign-jumps={jumps}",
        f"-falign-labels={labels}","-falign-loops=1",
        "-c",str(V/"src/SymbolTableUnit.cpp"),"-o",str(obj)]
   p=subprocess.run(cmd,capture_output=True,text=True)
   row={"jumps":jumps,"labels":labels,"compiler":snap}
   if p.returncode: row["compiler_error"]=p.stderr[-220:]
   else:
    result=subprocess.run(["nm","-S","--defined-only",str(obj)],capture_output=True,text=True)
    sym=next((l.split() for l in result.stdout.splitlines() if l.split()[-1:]==[cfg["function"]]),None)
    if sym:
     row["size"]=int(sym[1],16)
     if row["size"]==149:
      try:
       score=m.run(obj,newcfg)
       row["score"]=score["exact_bytes"];row["diffs"]=score["different_offsets"]
       row["relocs"]=score["relocations"];row["exact"]=score["exact"]
      except ValueError as ex:row["unverified"]=str(ex)
   print(json.dumps(row,sort_keys=True),flush=True);rows.append(row)
   if row.get("exact"):
    (O/"results.json").write_text(json.dumps(rows,indent=2)+"\n");raise SystemExit(0)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

#!/usr/bin/env python3
"""Original SymbolTable::Enter branch ownership and EH landing pad.

The genuine nested Node owner already matches the allocation-branch body,
hash calculation and linked-list update instructions. Under GCC 3.3
-falign-jumps=8, the entry branch and its five-byte alignment match
exactly, but the EH landing pad receives one extra NOP. Test normal 2004
C++03 guard-source layouts which may change GCC block placement.
No inline assembly, artificial padding, or invented instruction sequences.
"""
from pathlib import Path
import subprocess
import importlib.util
import json

ROOT=Path(__file__).resolve().parents[2]
VUC=ROOT/"vuc"
OUT=Path("/tmp/vuc-binutils215/symbol-enter-guard-flow-oct15m")
OUT.mkdir(parents=True, exist_ok=True)
spec=importlib.util.spec_from_file_location("match", VUC/"scripts/match_probe.py")
match=importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)

src=Path("/tmp/vuc-binutils215/symboltable-nested-node-oct15k/SymbolTableWithNestedNode.cpp").read_text()
method_start=src.index("void SymbolTable::Enter(")
method_end=src.index("SymbolTableEntry *SymbolTable::Find(",method_start)
method=src[method_start:method_end]
prefix=src[:method_start]
suffix=src[method_end:]
guard="    if (scopes_.empty()) return;\n"
assert method.count(guard)==1
body=method[method.index(guard)+len(guard):method.rfind("}")].strip("\n")
assert body.startswith("    SymbolScope &scope =")
forms={
 "original":method,
 "size-zero":method.replace(guard,"    if (scopes_.size() == 0) return;\n"),
 "begin-end":method.replace(guard,"    if (scopes_.begin() == scopes_.end()) return;\n"),
 "not-empty-then":method[:method.index(guard)]+"    if (!scopes_.empty()) {\n"+body+"\n    }\n}\n\n",
 "empty-else":method[:method.index(guard)]+"    if (scopes_.empty()) {\n        return;\n    } else {\n"+body+"\n    }\n}\n\n",
 "empty-goto-end":method.replace(guard,"    if (scopes_.empty()) goto finished;\n").replace("\n}\n","\nfinished:\n    ;\n}\n"),
 "not-empty-goto-body":method.replace(guard,"    if (!scopes_.empty()) goto have_scope;\n    return;\nhave_scope:\n"),
 "size-boolean":method.replace(guard,"    if (!scopes_.size()) return;\n"),
}
flags=["-O2","-frename-registers","-falign-jumps=8","-falign-loops=1","-falign-labels=1"]
opts=["symboltable_enter_oct13_probe.json","scope_vector_insert_partial_oct15i.json"]
symbol="_ZN11SymbolTable5EnterERK12MLGameStringP16SymbolTableEntry"
rows=[]
for name,definition in forms.items():
 cpp=OUT/(name+".cpp")
 cpp.write_text(prefix+definition+suffix)
 for cc in ("jan25","dec06"):
  obj=OUT/(cc+"-"+name+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+cc+"-as215",*flags,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"form":name,"compiler":cc}
  if cp.returncode:
   row["error"]=cp.stderr[-200:]
  else:
   nm=subprocess.check_output(["nm","-S","--defined-only",str(obj)],text=True)
   entry=next((x.split() for x in nm.splitlines() if x.endswith(" "+symbol)),None)
   row["enter_size"]=int(entry[1],16) if entry else None
   for label,config in zip(("enter","scope"),opts):
    try:
     r=match.run(obj,VUC/"config"/config)
     row[label]={"exact_bytes":r["exact_bytes"],"length":r["length"],
                  "relocations":r["relocations"],
                  "diff":r["different_offsets"][:20]}
    except ValueError as e:
     row[label]={"error":str(e)}
  rows.append(row)
  print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

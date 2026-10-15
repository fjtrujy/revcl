#!/usr/bin/env python3
"""Check original MLHashTable::Node alias type in SymbolTable::Enter.

Our provisional SymbolNode is a distinct struct cast to SymbolScope::Node*.
The genuine original likely stores the actual nested hash-node type, which
changes TBAA/alias assumptions and thus the order of old-head/new-head stores.
Keep node allocation and key copy authentic; only vary the final pointer
expression, never insert register-forcing asm.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

ROOT=Path(__file__).resolve().parents[2]
VUC=ROOT/"vuc"
OUT=Path("/tmp/vuc-binutils215/enter-node-alias-oct15k")
OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",VUC/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
unit=(VUC/"src/SymbolTableUnit.cpp").read_text()
for f in ("MLGameString.h","MLHashTable.h","MLHashFunGameString.h"):
 unit=unit.replace('#include "'+f+'"','#include "'+str(VUC/"src"/f)+'"')
needle="""    unsigned bucket = node->hash % 513;
    node->next = (SymbolNode *)scope.heads[bucket];
    scope.heads[bucket] = (SymbolScope::Node *)node;"""
assert needle in unit
sources={
"baseline":needle,
"actual-node-member":"""    unsigned bucket = node->hash % 513;
    SymbolScope::Node *actual = (SymbolScope::Node *)node;
    actual->next = scope.heads[bucket];
    scope.heads[bucket] = actual;""",
"actual-node-pointer":"""    unsigned bucket = node->hash % 513;
    SymbolScope::Node *actual = (SymbolScope::Node *)node;
    SymbolScope::Node **head = &scope.heads[bucket];
    actual->next = *head;
    *head = actual;""",
"actual-node-volatile-reference":"""    unsigned bucket = node->hash % 513;
    SymbolScope::Node *actual = (SymbolScope::Node *)node;
    SymbolScope::Node *volatile &head = scope.heads[bucket];
    actual->next = head;
    head = actual;""",
}
flags={
 "j1":["-O2","-frename-registers","-falign-jumps=1"],
 "j8-loop1":["-O2","-frename-registers","-falign-jumps=8","-falign-labels=1","-falign-loops=1"],
}
results=[]
# The actual nested Node store exposes an unnecessary source initializer:
# the provisional SymbolNode initializes next(0), although every path that
# publishes the node immediately replaces the link with the previous head.
# Test both authentic human-style constructor shapes, not dead-code padding.
for ctor_label,owner in {
    "initialized": unit,
    "deferred-next": unit.replace("value(v), next(0)", "value(v)"),
}.items():
 for name,block in sources.items():
  src=OUT/(ctor_label+"-"+name+".cpp")
  src.write_text(owner.replace(needle,block))
  for label,opts in flags.items():
   obj=OUT/(ctor_label+"-"+name+"-"+label+".o")
   cmd=["/tmp/vuc-binutils215/g++-jan25-as215",*opts,"-c",str(src),"-o",str(obj)]
   cp=subprocess.run(cmd,text=True,capture_output=True)
   row={"constructor":ctor_label,"case":name,"flags":label}
   if cp.returncode:
    row["error"]=cp.stderr[-250:]
   else:
    for target,config in (("enter","symboltable_enter_oct13_probe.json"),("scope","scope_vector_insert_partial_oct15i.json")):
     try:
      s=m.run(obj,VUC/"config"/config)
      row[target]={"match":s["exact_bytes"],"size":s["length"],"reloc":s["relocations"],"diff":s["different_offsets"]}
     except ValueError as e:
      row[target]={"error":str(e)}
   results.append(row)
   print(ctor_label,row["case"],row["flags"],{key:(val.get("match"),val.get("size"),val.get("error")) for key,val in row.items() if key in ("enter","scope")},row.get("error",""),flush=True)
(OUT/"results.json").write_text(json.dumps(results,indent=2)+"\n")

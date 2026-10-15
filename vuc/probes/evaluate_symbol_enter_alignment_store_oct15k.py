#!/usr/bin/env python3
"""Instruction-led SymbolTable::Enter hypotheses with real SymbolTable TU.

Original 185-byte Enter has 5 alignment bytes before allocation and writes
new->next before the bucket head. The provisional source yields 180 bytes
and writes bucket head first. Test only real compiler label/jump controls
and authentic C++03 bucket insertion expressions. Also score the naturally
emitted vector<SymbolScope> instantiation in the *same* object.
"""
from pathlib import Path
import itertools
import importlib.util
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/symbol-enter-alignment-store-oct15k")
OUT.mkdir(parents=True, exist_ok=True)
spec=importlib.util.spec_from_file_location("match", VUC / "scripts/match_probe.py")
match=importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
src=(VUC/"src/SymbolTableUnit.cpp").read_text()
src=src.replace('#include "MLGameString.h"', '#include "'+str(VUC/"src/MLGameString.h")+'"')
src=src.replace('#include "MLHashTable.h"', '#include "'+str(VUC/"src/MLHashTable.h")+'"')
src=src.replace('#include "MLHashFunGameString.h"', '#include "'+str(VUC/"src/MLHashFunGameString.h")+'"')
orig="""    node->next = (SymbolNode *)scope.heads[bucket];
    scope.heads[bucket] = (SymbolScope::Node *)node;"""
# Need source changes that alter scheduling, not useless register assignment.
variants={
    "current":orig,
    "old-before":"""    SymbolNode *old = (SymbolNode *)scope.heads[bucket];
    node->next = old;
    scope.heads[bucket] = (SymbolScope::Node *)node;""",
    "head-index":"""    SymbolScope::Node **head = &scope.heads[bucket];
    node->next = (SymbolNode *)*head;
    *head = (SymbolScope::Node *)node;""",
    "store-head-first":"""    SymbolNode *old = (SymbolNode *)scope.heads[bucket];
    scope.heads[bucket] = (SymbolScope::Node *)node;
    node->next = old;""",
}
assert orig in src
# Justify only options capable of supplying the 5 authentic align bytes
# at the observed original 16-byte allocation target.
alignments={
 "j1-l1": ["-falign-jumps=1","-falign-labels=1"],
 "j1-l16": ["-falign-jumps=1","-falign-labels=16"],
 "j2-l16": ["-falign-jumps=2","-falign-labels=16"],
 "j4-l16": ["-falign-jumps=4","-falign-labels=16"],
 "j8-l16": ["-falign-jumps=8","-falign-labels=16"],
 "j16-l1": ["-falign-jumps=16","-falign-labels=1"],
 "j16-l16": ["-falign-jumps=16","-falign-labels=16"],
}
rows=[]
best_enter=None
best_scope=None
for (name,replace),(flabel,align) in itertools.product(variants.items(),alignments.items()):
    tag=name+"-"+flabel
    cpp=OUT/(tag+".cpp")
    obj=OUT/(tag+".o")
    cpp.write_text(src.replace(orig, replace))
    p=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",
             "-O2","-frename-registers",*align,"-c",str(cpp),"-o",str(obj)],
             text=True,capture_output=True)
    if p.returncode:
        rows.append({"case":tag,"error":p.stderr[-160:]})
        continue
    row={"case":tag,"checks":{}}
    for label,config in {
        "enter":"symboltable_enter_oct13_probe.json",
        "scope":"scope_vector_insert_partial_oct15i.json",
        "find":"symboltable_find_probe.json",
        "begin":"symbol_begin_scope_probe.json",
        "end":"symboltable_end_exact_probe.json",
    }.items():
        try:
            s=match.run(obj, VUC/"config"/config)
            row["checks"][label]={"bytes":s["exact_bytes"],"size":s["length"],
                  "relocations":s["relocations"],"offsets":s["different_offsets"]}
        except ValueError as e:
            row["checks"][label]={"error":str(e)}
    rows.append(row)
    ent=row["checks"]["enter"]
    scope=row["checks"]["scope"]
    if ent.get("size")==185 and (best_enter is None or ent["bytes"]>best_enter["bytes"]):
        best_enter={"case":tag,**ent}
        print("BEST_ENTER",best_enter,flush=True)
    if scope.get("size")==746 and scope.get("relocations")==15 and (best_scope is None or scope["bytes"]>best_scope["bytes"]):
        best_scope={"case":tag,**scope}
        print("BEST_SCOPE",best_scope,flush=True)
    if ent.get("bytes")==185 or scope.get("bytes")==746:
        print("EXACT_CANDIDATE",tag,flush=True)
    print("CHECKED",tag,{n:(v.get("bytes"),v.get("size")) for n,v in row["checks"].items()},flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")
print("DONE",len(rows),best_enter,best_scope)

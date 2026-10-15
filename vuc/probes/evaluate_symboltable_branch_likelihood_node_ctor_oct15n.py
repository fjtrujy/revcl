#!/usr/bin/env python3
"""Focused original-era C++03 source controls for SymbolTable::Enter.

Original node constructor calls hash, copy-constructs the key and publishes
the linked list. Its jan25+dec06 GCC code differs only by a trailing EH
landing pad NOP when natural 8-byte allocation branch alignment is used.
Test branch likelihood and genuine ctor member lifetime, not broad flags.
Accept only authenticated original-function-size candidates.
"""
from pathlib import Path
import importlib.util
import itertools
import json
import subprocess

ROOT=Path(__file__).resolve().parents[2]; R=ROOT/"vuc"
OUT=Path("/tmp/vuc-binutils215/symboltable-branch-node-ctor-oct15n")
OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("match",R/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
unit=(R/"src/SymbolTableUnit.cpp").read_text()
for inc in ("MLGameString.h","MLHashTable.h","MLHashFunGameString.h"):
 unit=unit.replace('#include "'+inc+'"','#include "'+str(R/"src"/inc)+'"')
header=(R/"src/MLHashTable.h").read_text()
needle="""        Node(const Key &k, unsigned h, Value v)
            : key(k), hash(h), value(v) {}"""
assert needle in header
assert 'if (scopes_.empty()) return;' in unit

ctors={
  "member-inits":needle,
  "assignment-body":"""        Node(const Key &k, unsigned h, Value v)
            : key(k) { hash = h; value = v; }""",
  "hash-init-assign":"""        Node(const Key &k, unsigned h, Value v)
            : key(k), hash(h) { value = v; }""",
  "hash-assign-value-init":"""        Node(const Key &k, unsigned h, Value v)
            : key(k), value(v) { hash = h; }""",
  "value-constref":"""        Node(const Key &k, unsigned h, const Value &v)
            : key(k), hash(h), value(v) {}""",
}
guards={
 "normal":"if (scopes_.empty()) return;",
 "unlikely":"if (__builtin_expect(scopes_.empty(), 0)) return;",
 "likely":"if (__builtin_expect(scopes_.empty(), 1)) return;",
 "negated-likely":"if (__builtin_expect(!scopes_.empty(), 1)) {} else return;",
}
flags=["-O2","-frename-registers","-falign-jumps=8","-falign-loops=1","-falign-labels=1"]
cfgs={"enter":"symboltable_enter_oct13_probe.json","scope":"scope_vector_insert_partial_oct15i.json"}
rows=[]
for (constructor, ctor), (guard,condition) in itertools.product(ctors.items(),guards.items()):
 label=constructor+"-"+guard
 headerpath=OUT/(label+"-MLHashTable.h")
 headerpath.write_text(header.replace(needle,ctor))
 srcpath=OUT/(label+".cpp")
 original=unit.replace(str(R/"src/MLHashTable.h"),str(headerpath))
 srcpath.write_text(original.replace('if (scopes_.empty()) return;',condition))
 obj=OUT/(label+".o")
 comp=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*flags,"-c",str(srcpath),"-o",str(obj)],capture_output=True,text=True)
 row={"constructor":constructor,"guard":guard}
 if comp.returncode:
  row["error"]=comp.stderr[-220:]
 else:
  for name,cfg in cfgs.items():
   try:
    score=m.run(obj,R/"config"/cfg)
    row[name]={"length":score["length"],"matching":score["exact_bytes"],
               "relocs":score["relocations"],"diff":score["different_offsets"][:30]}
   except ValueError as e:
    row[name]={"error":str(e)}
 rows.append(row)
 print(label,{x:(y.get("length"),y.get("matching"),y.get("error")) for x,y in row.items() if x in cfgs},row.get("error",""),flush=True)
(OUT/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

#!/usr/bin/env python3
"""Model the original SymbolTable Entry node as the actual hash-table Node.

The current application-unit reconstruction allocates a distinct SymbolNode
and casts to the real hash table's Node. Original 2004 C++ would more likely
allocate the nested Node itself; test a plausible period Node constructor
without changing any accepted header. Compile authentic Enter/Find/BeginScope
and EndScope in their original source order and compare both weak templates.
"""
from pathlib import Path
import json
import importlib.util
import subprocess

ROOT=Path(__file__).resolve().parents[2]
VUC=ROOT/"vuc"
OUT=Path("/tmp/vuc-binutils215/symboltable-nested-node-oct15k")
OUT.mkdir(parents=True,exist_ok=True)
spec=importlib.util.spec_from_file_location("m",VUC/"scripts/match_probe.py")
m=importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

header=(VUC/"src/MLHashTable.h").read_text()
needle="""        Node *next;
    };"""
if needle in header:
    header=header.replace(needle,"""        Node *next;
        Node(const Key &k, unsigned h, Value v)
            : key(k), hash(h), value(v) {}
    };""")
else:
    assert "Node(const Key &k, unsigned h, Value v)" in header
header_path=OUT/"MLHashTableNodeOriginalHypothesis.h"
header_path.write_text(header)
unit=(VUC/"src/SymbolTableUnit.cpp").read_text()
for inc in ("MLGameString.h","MLHashFunGameString.h"):
    unit=unit.replace('#include "'+inc+'"','#include "'+str(VUC/"src"/inc)+'"')
unit=unit.replace('#include "MLHashTable.h"','#include "'+str(header_path)+'"')
if "// A hash entry stores" in unit:
    start=unit.index("// A hash entry stores")
    end=unit.index("class SymbolTable {",start)
    unit=unit[:start]+unit[end:]
    unit=unit.replace("SymbolNode *node = new SymbolNode(name, hasher.Hash(name), entry);",
                     "SymbolScope::Node *node = new SymbolScope::Node(name, hasher.Hash(name), entry);")
    unit=unit.replace("node->next = (SymbolNode *)scope.heads[bucket];",
                      "node->next = scope.heads[bucket];")
    unit=unit.replace("scope.heads[bucket] = (SymbolScope::Node *)node;",
                      "scope.heads[bucket] = node;")
assert "SymbolNode" not in unit
cpp=OUT/"SymbolTableWithNestedNode.cpp"
cpp.write_text(unit)
cases={
"j1":["-O2","-frename-registers","-falign-jumps=1"],
"j8-loop1":["-O2","-frename-registers","-falign-jumps=8","-falign-loops=1","-falign-labels=1"],
}
cfgs={"enter":"symboltable_enter_oct13_probe.json",
      "find":"symboltable_find_probe.json",
      "begin":"symbol_begin_scope_probe.json",
      "end":"symboltable_end_exact_probe.json",
      "scope":"scope_vector_insert_partial_oct15i.json"}
results=[]
for snap in ("jan25","dec06"):
 for label,flags in cases.items():
  obj=OUT/(snap+"-"+label+".o")
  cp=subprocess.run(["/tmp/vuc-binutils215/g++-"+snap+"-as215",
       *flags,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
  row={"snapshot":snap,"flags":label}
  if cp.returncode:
   row["error"]=cp.stderr[-200:]
  else:
   for name,fn in cfgs.items():
    try:
     score=m.run(obj,VUC/"config"/fn)
     row[name]={"matched":score["exact_bytes"],"length":score["length"],
                "relocations":score["relocations"],
                "different_offsets":score["different_offsets"][:20]}
    except ValueError as e:
     row[name]={"error":str(e)}
  results.append(row)
  print(row,flush=True)
(OUT/"results.json").write_text(json.dumps(results,indent=2)+"\n")

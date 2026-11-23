#!/usr/bin/env python3
"""Target a one-byte GCC 3.3.3 EH block-alignment discrepancy in SymbolTable::Enter.

The real nested SymbolScope::Node source generates 186 bytes under j8-l1:
its first 167 bytes agree with the original, but compiler places one extra
alignment NOP before the EH landing pad, where original has none.
Test historically valid optimizer settings that might change that label's
branch classification while preserving the early 5-byte alignment.
No asm, artificial padding, fake aliases, or original-ELF edits.
"""
from pathlib import Path
import json,importlib.util,subprocess
V=Path(__file__).resolve().parents[1]
O=Path("/tmp/vuc-symbol-enter-eh-align-20261123");O.mkdir(exist_ok=True)
head=(V/"src/MLHashTable.h").read_text()
old="""        Node *next;
    };"""
new="""        Node *next;
        Node(const Key &k, unsigned h, Value v) : key(k), hash(h), value(v) {}
    };"""
# Current tracked header already contains the original-shaped nested Node
# constructor (older diagnostic source snapshots did not).
assert old in head or "Node(const Key &k, unsigned h, Value v)" in head
h=O/"MLHashTableNode.h";h.write_text(head.replace(old,new))
unit=(V/"src/SymbolTableUnit.cpp").read_text()
unit=unit.replace('#include "MLHashTable.h"',f'#include "{h}"')
for f in ("MLGameString.h","MLHashFunGameString.h"):
    unit=unit.replace(f'#include "{f}"',f'#include "{V/"src"/f}"')
assert 'new SymbolScope::Node(name' in unit
src=O/"original-node.cpp";src.write_text(unit)
sp=importlib.util.spec_from_file_location("m",V/"scripts/match_probe.py")
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
variants={
    "baseline":[],
    "no-reorder":["-fno-reorder-blocks"],
    "no-crossjump":["-fno-crossjumping"],
    "no-branch-guess":["-fno-guess-branch-probability"],
    "no-if-conversion":["-fno-if-conversion"],
    "no-if-conversion2":["-fno-if-conversion2"],
    "no-sched2":["-fno-schedule-insns2"],
    "no-sched":["-fno-schedule-insns"],
    "no-peephole2":["-fno-peephole2"],
    "no-regmove":["-fno-regmove"],
    "no-gcse":["-fno-gcse"],
    "no-renaming":["-fno-rename-registers"],
    "no-cse-follow":["-fno-cse-follow-jumps"],
    "no-sibling":["-fno-optimize-sibling-calls"],
    "no-omit-frame":["-fno-omit-frame-pointer"],
    "no-inline":["-fno-inline"],
    "no-inline-fns":["-fno-inline-functions"],
    "no-delayed-branch":["-fno-delayed-branch"],
    "no-unit-at-a-time":["-fno-unit-at-a-time"],
    "no-align-functions":["-falign-functions=1"],
    "align-functions16":["-falign-functions=16"],
    "no-align-loops":["-falign-loops=1"],
    "align-loops2":["-falign-loops=2"],
    "no-rerun-cse":["-fno-rerun-cse-after-loop"],
    "no-strict-alias":["-fno-strict-aliasing"],
    "no-expensive":["-fno-expensive-optimizations"],
}
cfgs={
    "enter":"symboltable_enter_oct13_probe.json",
    "find":"symboltable_find_probe.json",
    "begin":"symbol_begin_scope_probe.json",
    "end":"symboltable_end_exact_probe.json",
    "vector":"scope_vector_insert_partial_oct15i.json",
}
rows=[]
for name,flags in variants.items():
  for compiler in ("jan25","dec06"):
    obj=O/(name+"-"+compiler+".o")
    args=["/tmp/vuc-binutils215/g++-"+compiler+"-as215",
         "-O2","-frename-registers","-falign-jumps=8","-falign-labels=1",
         "-falign-loops=1",*flags,"-c",str(src),"-o",str(obj)]
    cp=subprocess.run(args,capture_output=True,text=True)
    row={"variant":name,"compiler":compiler}
    if cp.returncode:row["error"]=cp.stderr[-420:]
    else:
      for label,config in cfgs.items():
        try:
          v=m.run(obj,V/"config"/config)
          row[label]={"size":v["length"],"matched":v["exact_bytes"],"exact":v["exact"],
                      "diffs":v["different_offsets"][:22],"relocs":v["relocations"]}
        except (ValueError,KeyError) as ex:row[label]={"error":str(ex)}
    print(json.dumps(row,sort_keys=True),flush=True)
    rows.append(row)
(O/"results.json").write_text(json.dumps(rows,indent=2)+"\n")

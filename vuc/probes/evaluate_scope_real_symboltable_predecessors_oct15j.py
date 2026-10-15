#!/usr/bin/env python3
"""Scope vector growth: true SymbolTable method predecessors, not flag sweeps.

The original SymbolTable.cpp order is Enter -> Find -> BeginScope -> EndScope.
Test plausible actual Enter implementations with the exact reconstructed Find
and BeginScope, compiling their naturally emitted weak vector instantiation
under the one historical alignment already known to preserve 746 bytes.
No inline asm, artificial predecessor functions, manual padding, or binaries
modified. The original-offset relocation-aware gate is mandatory.
"""
from pathlib import Path
import importlib.util
import itertools
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/scope-symboltable-predecessors-oct15j")
OUT.mkdir(parents=True, exist_ok=True)

spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
unit = (VUC / "src/SymbolTableUnit.cpp").read_text()
for include in ("MLGameString.h", "MLHashTable.h", "MLHashFunGameString.h"):
    unit = unit.replace('#include "' + include + '"',
                        '#include "' + str(VUC / "src" / include) + '"')
begin = unit.index("void SymbolTable::Enter(")
end = unit.index("SymbolTableEntry *SymbolTable::Find(", begin)
start = unit[:begin]
old_enter = unit[begin:end]
continuation = unit[end:]
baseline = """    SymbolNode *node = new SymbolNode(name, hasher.Hash(name), entry);"""
slot = """    unsigned bucket = node->hash % 513;
    node->next = (SymbolNode *)scope.heads[bucket];
    scope.heads[bucket] = (SymbolScope::Node *)node;"""
assert baseline in old_enter and slot in old_enter

# Real-world source alternatives already proposed for this incomplete 185-byte
# original predecessor. Do not manufacture functions to tweak compiler state.
constructor_shapes = {
    "original": baseline,
    "key-ref": """    const MLGameString &key = name;
    SymbolNode *node = new SymbolNode(key, hasher.Hash(key), entry);""",
    "entry-ref": """    SymbolTableEntry *const &value = entry;
    SymbolNode *node = new SymbolNode(name, hasher.Hash(name), value);""",
    "node-late": """    SymbolNode *node;
    node = new SymbolNode(name, hasher.Hash(name), entry);""",
    "named-hash": """    unsigned hash = hasher.Hash(name);
    SymbolNode *node = new SymbolNode(name, hash, entry);""",
}
bucket_shapes = {
    "original": slot,
    "slot-reference": """    SymbolScope::Node *&destination = scope.heads[node->hash % 513];
    node->next = (SymbolNode *)destination;
    destination = (SymbolScope::Node *)node;""",
    "slot-pointer": """    SymbolScope::Node **destination = &scope.heads[node->hash % 513];
    node->next = (SymbolNode *)*destination;
    *destination = (SymbolScope::Node *)node;""",
}
# Baseline authentic 2004 GCC options are fixed, so results attribute changes to
# translation-unit source, not an undirected search over optimizer switches.
flags = ["-O2", "-frename-registers", "-falign-jumps=1"]
configs = {
    "scope_insert": VUC / "config/scope_vector_insert_partial_oct15i.json",
    "enter": VUC / "config/symboltable_enter_oct13_probe.json",
    "find": VUC / "config/symboltable_find_probe.json",
}
rows = []
best = None
for (ctor_name, ctor), (slot_name, destination) in itertools.product(
        constructor_shapes.items(), bucket_shapes.items()):
    tag = ctor_name + "-" + slot_name
    body = start + old_enter.replace(baseline, ctor).replace(slot, destination) + continuation
    cpp = OUT / (tag + ".cpp")
    obj = OUT / (tag + ".o")
    cpp.write_text(body)
    compiled = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *flags,
                               "-c", str(cpp), "-o", str(obj)],
                              text=True, capture_output=True)
    if compiled.returncode:
        rows.append({"name": tag, "error": compiled.stderr[-400:]})
        continue
    record = {"name": tag, "score": {}}
    for label, path in configs.items():
        try:
            score = matcher.run(obj, path)
            record["score"][label] = {"length": score["length"],
                                      "matching": score["exact_bytes"],
                                      "relocations": score["relocations"],
                                      "mismatches": score["different_offsets"][:25]}
        except ValueError as error:
            record["score"][label] = {"error": str(error)}
    rows.append(record)
    scope = record["score"]["scope_insert"]
    if scope.get("length") == 746 and scope.get("relocations") == 15:
        if best is None or scope["matching"] > best["score"]["scope_insert"]["matching"]:
            best = record
            (OUT / "best.cpp").write_text(body)
            (OUT / "best.o").write_bytes(obj.read_bytes())
            print("BEST", tag, scope["matching"], "/746", flush=True)
        if scope["matching"] == 746:
            print("EXACT", tag, flush=True)
            break
    print("CHECKED", tag,
          {name: (value.get("matching"), value.get("length"))
           for name, value in record["score"].items()}, flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
print("DONE", len(rows), "best:", best["name"] if best else None,
      best["score"]["scope_insert"] if best else None)

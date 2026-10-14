#!/usr/bin/env python3
"""Original VUC 166-byte MLHashTable copy-ctor source/CFG reconstruction.

Reproduces historical C++ node copying, linked-list lifetime and array
bucket traversal. Measures only original-sized relocations with the strict
original ELF comparator. No generated objects enter Git.
"""
from pathlib import Path
from itertools import product
import subprocess
import importlib.util

ROOT = Path(__file__).resolve().parents[2]
T = Path("/tmp/vuc-binutils215/hash-copy-ctor-oct14")
T.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("match", ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
original = (ROOT / "vuc/probes/hash_copy_constructor.cpp").read_text()
start = original.index("    for (int i = 0; i < Buckets; ++i)")
end = original.index("\n}", start)
body = original[start:end]
forms = {"baseline": body}
forms["unsigned-index"] = body.replace("int i", "unsigned i")
forms["short-index"] = body.replace("int i", "short i")
forms["while-buckets"] = body.replace(
    "for (int i = 0; i < Buckets; ++i) {",
    "int i = 0;\n    while (i < Buckets) {").replace("        }\n    }", "        }\n        ++i;\n    }")
forms["preinc-buckets"] = body.replace("++i", "i++")
forms["bucket-address"] = body.replace(
    "heads[i] = 0;\n        Node **tail = &heads[i];",
    "Node **tail = &heads[i];\n        *tail = 0;")
forms["copy-pointee"] = body.replace("new Node(*node)", "new Node(node->key, node->hash, node->value, node->next)")
# Node has no 4-argument constructor: skip this form deliberately.
del forms["copy-pointee"]
forms["separate-current"] = body.replace(
    "for (Node *node = other.heads[i]; node; node = node->next) {",
    "Node *node = other.heads[i];\n        while (node) {\n"
    "            Node *next = node->next;").replace(
    "tail = &copy->next;\n        }",
    "tail = &copy->next;\n            node = next;\n        }")
forms["tail-link-after"] = body.replace(
    "copy->next = 0;\n            *tail = copy;",
    "*tail = copy;\n            copy->next = 0;")
forms["tail-from-copy"] = body.replace(
    "tail = &copy->next;", "tail = &(copy->next);")
flags = {}
for ren,jump,loop in product((True,False),(1,2,4,8,16),(1,4,8,16)):
    key = ("r" if ren else "p") + "-j%d-l%d" % (jump,loop)
    flags[key] = ["-O2", "-falign-jumps=%d" % jump, "-falign-loops=%d" % loop] + (
        ["-frename-registers"] if ren else [])
rows=[]
for name, function_body in forms.items():
    source = original[:start]+function_body+original[end:]
    cpp = T/(name+".cpp"); cpp.write_text(source.replace(
       '#include "../src/MLHashTable.h"', '#include "'+str(ROOT/'vuc/src/MLHashTable.h')+'"').replace(
       '#include "../src/MLHashFunGameString.h"', '#include "'+str(ROOT/'vuc/src/MLHashFunGameString.h')+'"'))
    for flag, opts in flags.items():
        obj = T/(name+"-"+flag+".o")
        cp = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",
                             *opts,"-c",str(cpp),"-o",str(obj)],capture_output=True,text=True)
        if cp.returncode:
            print("ERROR",name,flag,cp.stderr[-100:],flush=True);continue
        line = next((x for x in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines()
                     if x.endswith("_ZN11MLHashTableI12MLGameStringP16SymbolTableEntry9MLHashFunIS0_ELi513EEC1ERKS5_")),"")
        size=int(line.split()[1],16) if line else -1
        if size != 166:
            rows.append((0,size,name,flag,"length"));continue
        try:
            score=match.run(obj,ROOT/"vuc/config/hash_copy_constructor_probe.json")
            rows.append((score["exact_bytes"],size,name,flag,score["different_offsets"][:12]))
            if score["exact"]:
                print("NEW_EXACT",name,flag,cpp,flush=True)
                raise SystemExit(0)
        except ValueError as e:
            rows.append((0,size,name,flag,"reject "+str(e)[:100]))
    print("CHECKED",name,flush=True)
print("BEST")
for item in sorted(rows,reverse=True)[:30]:print(item,flush=True)

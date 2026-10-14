#!/usr/bin/env python3
"""Source-level layout/lifetime probes for 256-byte VUC ASTExpr list translator."""
import importlib.util
import itertools
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/expr-list-oct14")
TMP.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
base = (ROOT / "vuc/probes/translate_expr_list_oct14.cpp").read_text()
config = ROOT / "vuc/config/translate_expr_list_oct14_probe.json"
begin = base.index("    void Add(T element) {")
end = base.index("\n};", begin)
baseline = base[begin:end]
variants = {
    "baseline": baseline,
    "assign-new-first": baseline.replace(
        "T *old_entries = entries;\n            entries = new T[new_capacity];",
        "T *old_entries = entries;\n            T *fresh = new T[new_capacity];\n            entries = fresh;"),
    "assign-last": baseline.replace(
        "entries = new T[new_capacity];\n            if (old_entries)\n"
        "                __builtin_memcpy(entries, old_entries, length * sizeof(T));",
        "T *fresh = new T[new_capacity];\n            if (old_entries)\n"
        "                __builtin_memcpy(fresh, old_entries, length * sizeof(T));\n"
        "            entries = fresh;"),
    "capacity-last": baseline.replace(
        "capacity = new_capacity;\n        }",
        "capacity = new_capacity;\n        }\n"),
    "assign-capacity-first": baseline.replace(
        "T *old_entries = entries;\n            entries = new T[new_capacity];",
        "T *old_entries = entries;\n            capacity = new_capacity;\n            entries = new T[new_capacity];").replace(
        "            capacity = new_capacity;\n        }", "        }"),
    "local-count": baseline.replace(
        "if (length >= capacity) {",
        "int old_size = length;\n        if (old_size >= capacity) {").replace(
        "length * sizeof(T)", "old_size * sizeof(T)").replace(
        "entries[length++] = element;", "entries[old_size] = element;\n        length = old_size + 1;"),
    "manual-copy": baseline.replace(
        "__builtin_memcpy(entries, old_entries, length * sizeof(T));",
        "for (int j=0; j<length; ++j) entries[j] = old_entries[j];"),
}
outer = {
    "for": None,
    "while": ("    int i = 0;\n    while (i < input.length) {\n"
              "        result.Add(Translate(input.entries[i], 0x3c00, 0));\n"
              "        ++i;\n    }"),
    "local-result": ("    for (int i = 0; i < input.length; ++i) {\n"
                      "        IR::Expr_Data *expr = Translate(input.entries[i], 0x3c00, 0);\n"
                      "        result.Add(expr);\n    }"),
    "postinc": ("    int i=0;\n    while(i<input.length)\n"
                "        result.Add(Translate(input.entries[i++], 0x3c00, 0));"),
}
loop_original = ("    for (int i = 0; i < input.length; ++i)\n"
                 "        result.Add(Translate(input.entries[i], 0x3c00, 0));")
flags = {
  "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
  "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
  "j8": ["-O2", "-frename-registers", "-falign-jumps=8"],
  "no-reorder": ["-O2", "-frename-registers", "-falign-jumps=1", "-fno-reorder-blocks"],
  "no-reorder-j4": ["-O2", "-frename-registers", "-falign-jumps=4", "-fno-reorder-blocks"],
  "plain-no-reorder": ["-O2", "-falign-jumps=1", "-fno-reorder-blocks"]
}
entries=[]
for (name,add), (loop,block) in itertools.product(variants.items(),outer.items()):
    source = base[:begin]+add+base[end:]
    if block is not None:source=source.replace(loop_original,block)
    src=TMP/(name+"-"+loop+".cpp");src.write_text(source)
    for flag,opts in flags.items():
        obj=TMP/(name+"-"+loop+"-"+flag+".o")
        p=subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",*opts,
                           "-c",str(src),"-o",str(obj)],capture_output=True,text=True)
        if p.returncode:
            print("ERROR",name,loop,flag,p.stderr[-100:],flush=True);continue
        symbol=next((s for s in subprocess.check_output(["nm","-S",str(obj)],text=True).splitlines()
                     if s.endswith("_Z9TranslateR11ASTNodeListIP7ASTExprE")),"")
        size=int(symbol.split()[1],16) if symbol else -1
        if size==256:
            try:
                score=match.run(obj,config)
                entries.append((score["exact_bytes"],size,name,loop,flag,score["different_offsets"][:12]))
                if score["exact"]:print("NEW_EXACT",src,opts,flush=True);raise SystemExit(0)
            except ValueError as e:entries.append((0,size,name,loop,flag,str(e)[:110]))
        else:entries.append((0,size,name,loop,flag,"length"))
    print("DONE",name,loop,flush=True)
print("TOP")
for r in sorted(entries,reverse=True)[:35]:print(r,flush=True)

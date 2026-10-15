#!/usr/bin/env python3
"""Try period-plausible, sometimes redundant AST callee access idioms.

Original ASTFunCall printer (98 bytes) differs from the baseline by exactly
two register-selection bytes: its callee pointer uses ECX, not EAX.
Do not force hardware registers or insert code merely for padding.
"""
from pathlib import Path
import importlib.util
import itertools
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/printer-human-idioms-oct15h")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg = VUC / "config/print_ast_fun_context_oct14_probe.json"
base = (VUC / "src/Printer.cpp").read_text()
base = base.replace('#include "../src/ASTNodeList.h"', '#include "' + str(VUC / "src/ASTNodeList.h") + '"')
needle = "    Print(call->callee, output);"
assert needle in base
forms = {
    "direct": needle,
    "register-local": "    register ASTLValExpr *callee = call->callee;\n    Print(callee, output);",
    "register-const-local": "    register ASTLValExpr *const callee = call->callee;\n    Print(callee, output);",
    "old-cast": "    Print((ASTLValExpr *)call->callee, output);",
    "pointer-member": "    ASTLValExpr *ASTFunCall::*member = &ASTFunCall::callee;\n    Print(call->*member, output);",
    "register-member": "    register ASTLValExpr *ASTFunCall::*member = &ASTFunCall::callee;\n    Print(call->*member, output);",
    "member-address": "    ASTLValExpr **callee = &(call->callee);\n    Print(*callee, output);",
    "member-old-cast": "    ASTLValExpr **callee = (ASTLValExpr **)((char *)call + 8);\n    Print(*callee, output);",
    "selected-ternary": "    Print(call->callee ? call->callee : call->callee, output);",
    "misleading-double-assign": "    ASTLValExpr *callee = call->callee;\n    callee = call->callee;\n    Print(callee, output);",
    "unused-adjacent": "    ASTLValExpr *callee = call->callee;\n    ASTNodeList<ASTExpr *> &args = call->arguments;\n    Print(callee, output);",
    "read-output-first": "    std::ostream &stream = output;\n    ASTLValExpr *callee = call->callee;\n    Print(callee, stream);",
    "borrow-pointer": "    const ASTFunCall *ref = call;\n    Print(ref->callee, output);",
    "char-offset": "    Print(*(ASTLValExpr **)((char *)call + 8), output);",
}
flags = {
    "o2": ["-O2"],
    "ren": ["-O2", "-frename-registers"],
    "o2-j1": ["-O2", "-falign-jumps=1"],
    "ren-j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "o2-regmove": ["-O2", "-fno-regmove"],
    "o2-sched": ["-O2", "-fno-schedule-insns2"],
    "o2-web": ["-O2", "-fweb"],
}
rows = []
best = None
for (name, idiom), (flag, options) in itertools.product(forms.items(), flags.items()):
    tag = f"{name}-{flag}"
    content = base.replace(needle, idiom)
    cpp = OUT / (tag + ".cpp")
    obj = OUT / (tag + ".o")
    cpp.write_text(content)
    p = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *options,
                        "-c", str(cpp), "-o", str(obj)], text=True, capture_output=True)
    if p.returncode:
        rows.append({"tag": tag, "error": p.stderr[-140:]})
        continue
    try:
        score = matcher.run(obj, cfg)
    except ValueError as e:
        rows.append({"tag": tag, "error": str(e)})
        continue
    row = {"tag": tag, "length": score["length"], "match": score["exact_bytes"],
           "relocations": score["relocations"], "literal_bytes": score["verified_literal_bytes"],
           "different_offsets": score["different_offsets"]}
    rows.append(row)
    if (row["length"], row["relocations"], row["literal_bytes"]) != (98, 7, 9):
        continue
    if best is None or row["match"] > best["match"]:
        best = row
        (OUT / "best.cpp").write_text(content)
        (OUT / "best.flags").write_text(" ".join(options) + "\n")
        (OUT / "best.o").write_bytes(obj.read_bytes())
        print("BEST", json.dumps(row), flush=True)
    if row["match"] == 98:
        break
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
print(json.dumps({"attempted": len(rows),
                  "strict": sum(x.get("length") == 98 and x.get("relocations") == 7 and x.get("literal_bytes") == 9 for x in rows),
                  "best": best}, indent=2))

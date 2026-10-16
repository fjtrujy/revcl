#!/usr/bin/env python3
"""Test original ELF-observed Canonize_FoldExpr branch precedence.

Original VUC 0x080502c0 first checks Expr_Data kind=5 (binary) and then
kind=7 (constant). The initial source draft checks those in reverse.
These candidates express the original behavior with real C++03 source;
wrong-sized code remains diagnostic, not an accepted partial.
"""
from pathlib import Path
import subprocess
import json

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-binutils215/canonize-fold-original-branch-oct16")
O.mkdir(parents=True, exist_ok=True)
base = (R / "probes/CanonizeFoldExprNewOct15p.cpp").read_text()
base = base.replace('#include "../src/MLGameString.h"',
                    '#include "' + str(R / "src/MLGameString.h") + '"')
start = base.index('    if(child->kind==7) {')
end = base.index('    IR::BinOp_Data *binary=', start)
head, tail = base[:start], base[end:]
constant = '''
        IR::ConstLit_Data *copy=new IR::ConstLit_Data(((IR::ConstLit_Data*)child)->name);
        return new IR::ConstExpr_Data(wrapper->op,copy);
'''
error = '''
        error=true;
        std::cout << "illegal operand in const expr!";
        return expr;
'''
variants = {
    "binary-first-nested":
        '    if(child->kind!=5) {\n'
        '        if(child->kind!=7) {' + error + '        }\n' +
        constant + '    }\n',
    "binary-first-inverted":
        '    if(child->kind==5) { }\n'
        '    else if(child->kind==7) {' + constant + '    }\n'
        '    else {' + error + '    }\n',
    "binary-first-guard":
        '    if(child->kind!=5 && child->kind!=7) {' + error + '    }\n'
        '    if(child->kind==7) {' + constant + '    }\n',
    "switch-kind":
        '    switch(child->kind) {\n'
        '    case 5: break;\n'
        '    case 7: {' + constant + '    }\n'
        '    default: {' + error + '    }\n'
        '    }\n',
    "baseline": base[start:end],
}
rows = []
for name, replacement in variants.items():
    cpp = O / (name + ".cpp")
    cpp.write_text(head + replacement + tail)
    for flagname, flags in (
        ("O2", ["-O2"]),
        ("rename", ["-O2", "-frename-registers"]),
        ("jump4", ["-O2", "-frename-registers", "-falign-jumps=4"]),
    ):
        obj = O / (name + "-" + flagname + ".o")
        cmd = ["/tmp/vuc-binutils215/g++-jan25-as215",
               *flags, "-c", str(cpp), "-o", str(obj)]
        run = subprocess.run(cmd, capture_output=True, text=True)
        row = {"variant": name, "flags": flagname}
        if run.returncode:
            row["error"] = run.stderr[-230:]
        else:
            nm = subprocess.run(["nm", "-S", "--defined-only", str(obj)],
                                capture_output=True, text=True)
            records = [line.split() for line in nm.stdout.splitlines()
                       if line.endswith(" _Z17Canonize_FoldExprPN2IR9Expr_DataE")]
            row["size"] = int(records[0][1], 16) if records else None
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(O / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

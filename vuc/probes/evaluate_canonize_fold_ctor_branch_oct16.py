#!/usr/bin/env python3
"""Combine original ELF branch order with genuine IR base-ctor ownership.

The original writes Expr_Data::kind before the derived vptr on both new
constant objects. Test ordinary 2004-era C++ constructor designs, without
manual stores or object-byte manipulation.
"""
import json
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-binutils215/canonize-fold-ctor-branch-oct16")
O.mkdir(parents=True, exist_ok=True)
base = (R / "probes/CanonizeFoldExprNewOct15p.cpp").read_text()
base = base.replace('#include "../src/MLGameString.h"',
                    '#include "' + str(R / "src/MLGameString.h") + '"')
start = base.index('    if(child->kind==7) {')
end = base.index('    IR::BinOp_Data *binary=', start)
new_branch = '''    if (child->kind != 5) {
        if (child->kind != 7) {
            error = true;
            std::cout << "illegal operand in const expr!";
            return expr;
        }
        IR::ConstLit_Data *copy=new IR::ConstLit_Data(((IR::ConstLit_Data*)child)->name);
        return new IR::ConstExpr_Data(wrapper->op,copy);
    }
'''
base = base[:start] + new_branch + base[end:]
old_lit = 'explicit ConstLit_Data(const MLGameString &v):name(v) {kind=7;}'
old_expr = 'ConstExpr_Data(Operator_Data *o,Expr_Data *e):op(o),expr(e) {kind=6;}'
variants = {"baseline": base}
for construct in ("initializer", "body", "derived-member-body", "late-kind"):
    code = base.replace('    unsigned kind;\n};',
                        '    unsigned kind;\n    Expr_Data():kind(0) {}\n'
                        + ('    explicit Expr_Data(unsigned k):kind(k) {}\n'
                           if construct != "body" else
                           '    explicit Expr_Data(unsigned k) {kind=k;}\n')
                        + '};')
    if construct in ("initializer", "body"):
        code = code.replace(old_lit, 'explicit ConstLit_Data(const MLGameString &v):Expr_Data(7),name(v) {}')
        code = code.replace(old_expr, 'ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6),op(o),expr(e) {}')
    elif construct == "derived-member-body":
        code = code.replace(old_lit, 'explicit ConstLit_Data(const MLGameString &v):Expr_Data(7) {name=v;}')
        code = code.replace(old_expr, 'ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6) {op=o;expr=e;}')
    elif construct == "late-kind":
        code = code.replace(old_lit, 'explicit ConstLit_Data(const MLGameString &v):Expr_Data(7),name(v) {kind=7;}')
        code = code.replace(old_expr, 'ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6),op(o),expr(e) {kind=6;}')
    variants[construct] = code
rows = []
for name, code in variants.items():
    cpp = O / (name + ".cpp")
    cpp.write_text(code)
    for fn, flags in (
        ("plain", ["-O2"]),
        ("rename", ["-O2", "-frename-registers"]),
        ("j8", ["-O2", "-frename-registers", "-falign-jumps=8"]),
    ):
        obj = O / (name + "-" + fn + ".o")
        run = subprocess.run(
            ["/tmp/vuc-binutils215/g++-jan25-as215", *flags,
             "-c", str(cpp), "-o", str(obj)],
            capture_output=True, text=True)
        row = {"variant": name, "flags": fn}
        if run.returncode:
            row["error"] = run.stderr[-180:]
        else:
            syms = subprocess.run(["nm", "-S", "--defined-only", str(obj)],
                                  capture_output=True, text=True)
            hits = [l.split() for l in syms.stdout.splitlines()
                    if l.endswith(" _Z17Canonize_FoldExprPN2IR9Expr_DataE")]
            row["size"] = int(hits[0][1], 16) if hits else None
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(O / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

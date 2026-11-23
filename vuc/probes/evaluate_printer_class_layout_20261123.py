#!/usr/bin/env python3
"""Evidence-driven native source layouts for the 96/98 VUC ASTFunCall printer.

Hypothesis: the current flat provisional struct may be hiding a genuine AST
base class and hence different alias/SSA/register lifetimes. Run only real
class/field representations that keep callee at +8 and args at +12 in i386.
Never mutate the accepted source or accept partials as exact matches.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-printer-class-layout-20261123")
OUT.mkdir(exist_ok=True)
src = (ROOT / "src/Printer.cpp").read_text()
src = src.replace('#include "../src/ASTNodeList.h"',
                  '#include "' + str(ROOT / "src/ASTNodeList.h") + '"')
original = """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTFunCall {
    int type;
    int attributes;
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};"""
assert original in src

layouts = {
    "flat-original": original,
    "base-two-fields": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { int type; int attributes; };
struct ASTFunCall : ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "base-typed-enum": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { enum Kind { Other = 0 }; Kind type; int attributes; };
struct ASTFunCall : ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "base-one-field": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { unsigned type; };
struct ASTFunCall : ASTNode {
    unsigned attributes;
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "derived-as-expr": """struct ASTExpr { unsigned type; unsigned flags; };
struct ASTLValExpr {};
struct ASTFunCall : ASTExpr {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-ast-base": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { virtual ~ASTNode(); unsigned kind; };
struct ASTFunCall : ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-expr-base": """struct ASTExpr { virtual ~ASTExpr(); unsigned kind; };
struct ASTLValExpr {};
struct ASTFunCall : ASTExpr {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-method-base": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { virtual void check(); unsigned kind; };
struct ASTFunCall : ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "base-two-char-fields": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { unsigned short kind; unsigned short size; unsigned attributes; };
struct ASTFunCall : ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "base-two-pointers": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { void *kind; void *flags; };
struct ASTFunCall : ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "single-base-pointer": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { void *kind; };
struct ASTFunCall : ASTNode {
    int attributes;
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "two-base-levels": """struct ASTExpr {};
struct ASTLValExpr {};
struct ASTNode { unsigned type; };
struct ASTExpression : ASTNode { unsigned attributes; };
struct ASTFunCall : ASTExpression {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "private-base": """struct ASTExpr {};
struct ASTLValExpr {};
class ASTNode { int type; int attributes; };
struct ASTFunCall : ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
}

matcher_spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(matcher_spec)
matcher_spec.loader.exec_module(matcher)
config = ROOT / "config/print_ast_fun_context_oct14_probe.json"
rows = []
for tag, layout in layouts.items():
    path = OUT / (tag + ".cpp")
    path.write_text(src.replace(original, layout))
    for compiler in ("jan25", "dec06"):
        obj = OUT / (tag + "-" + compiler + ".o")
        command = ["/tmp/vuc-binutils215/g++-" + compiler + "-as215",
                   "-O2", "-c", str(path), "-o", str(obj)]
        result = subprocess.run(command, capture_output=True, text=True)
        row = {"variant": tag, "compiler": compiler}
        if result.returncode:
            row["error"] = result.stderr[-380:]
        else:
            try:
                measured = matcher.run(obj, config)
                row.update(length=measured["length"],
                           exact_bytes=measured["exact_bytes"],
                           different_offsets=measured["different_offsets"],
                           verified_relocations=measured["relocations"],
                           exact=measured["exact"])
            except (ValueError, KeyError) as error:
                row["not_authentic"] = str(error)
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

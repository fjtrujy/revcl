#!/usr/bin/env python3
"""Test RTTI-supported ASTFunCall inheritance against the original printer.

The original RTTI identifies ASTFunCall as deriving from ASTNode. The
provisional flat struct has the right offsets but not that source shape.
All variants retain real callee and argument-list fields and original literals.
"""
import importlib.util
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/printer-ast-layout-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
source = (ROOT / "vuc/src/Printer.cpp").read_text()
source = source.replace('"../src/ASTNodeList.h"', '"' + str(ROOT / "vuc/src/ASTNodeList.h") + '"')
old = """struct ASTFunCall {
    int type;
    int attributes;
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};"""
assert old in source
variants = {
    "flat": old,
    "plain-base": """struct ASTNode { unsigned reserved[2]; };
struct ASTFunCall : public ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-base": """struct ASTNode { virtual void apa(); unsigned reserved; };
struct ASTFunCall : public ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-destructor": """struct ASTNode { virtual ~ASTNode(); unsigned reserved; };
struct ASTFunCall : public ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-const": """struct ASTNode { virtual void apa() const; unsigned reserved; };
struct ASTFunCall : public ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-field-first": """struct ASTNode { unsigned reserved; virtual void apa(); };
struct ASTFunCall : public ASTNode {
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
    "virtual-two-parents": """struct ASTNode { virtual void apa(); };
struct ASTFunCall : public ASTNode {
    int flags;
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> arguments;
};""",
}
# Keep the independently verified four literal strings in the original unit.
flags = {
    "plain": ["-O2"],
    "rename": ["-O2", "-frename-registers"],
    "rename-j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "plain-j4": ["-O2", "-falign-jumps=4"],
}
for name, decl in variants.items():
    src = SCRATCH / (name + ".cpp")
    src.write_text(source.replace(old, decl))
    for f_name, opts in flags.items():
        obj = SCRATCH / (name + "-" + f_name + ".o")
        build = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(src), "-o", str(obj)], capture_output=True, text=True)
        if build.returncode:
            print("COMPILE_FAIL", name, f_name, build.stderr[-100:], flush=True)
            continue
        row = [name, f_name]
        for label, config in (
            ("call", "print_ast_fun_context_oct14_probe.json"),
            ("module", "ugly_printer_unit_oct13_probe.json"),
        ):
            try:
                result = matcher.run(obj, ROOT / "vuc/config" / config)
                row.append(f"{label}={result['exact_bytes']}/{result['length']}")
                if label == "call" and result["exact"]:
                    print("CALL_EXACT", name, f_name, src, flush=True)
            except ValueError as error:
                row.append(f"{label}=REJECT:{str(error)[:55]}")
        print(" ".join(row), flush=True)

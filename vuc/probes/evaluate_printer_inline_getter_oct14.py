#!/usr/bin/env python3
"""Test genuine inlined AST accessors as an explanation for printer registers.

Original target is 98 bytes and the historical binary loads call->callee
through ECX rather than the EAX currently chosen by GCC for plain field access.
No machine-code instructions or artificial predecessors are introduced.
"""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/printer-inline-getter-oct14")
TMP.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("match", ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
source = (ROOT / "vuc/src/Printer.cpp").read_text()
header = """    ASTNodeList<ASTExpr *> arguments;
"""
body = """    Print(call->callee, output);
    output << "(";
    Print(call->arguments, output, ", ");
    output << ")";
"""
assert header in source and body in source
cases = {
    "baseline": ("", body),
    "callee-inline": (
        "    inline ASTLValExpr *getCallee() { return callee; }\n",
        body.replace("call->callee", "call->getCallee()")),
    "callee-inline-const": (
        "    inline ASTLValExpr *getCallee() const { return callee; }\n",
        body.replace("call->callee", "call->getCallee()")),
    "args-inline": (
        "    inline ASTNodeList<ASTExpr *> &getArguments() { return arguments; }\n",
        body.replace("call->arguments", "call->getArguments()")),
    "both-inline": (
        "    inline ASTLValExpr *getCallee() { return callee; }\n"
        "    inline ASTNodeList<ASTExpr *> &getArguments() { return arguments; }\n",
        body.replace("call->callee", "call->getCallee()")
            .replace("call->arguments", "call->getArguments()")),
    "callee-deref": (
        "    inline ASTLValExpr &getCallee() { return *callee; }\n",
        body.replace("call->callee", "&call->getCallee()")),
    "callee-ref-const": (
        "    inline ASTLValExpr *const &getCallee() const { return callee; }\n",
        body.replace("call->callee", "call->getCallee()")),
    "args-ptr": (
        "    inline ASTNodeList<ASTExpr *> *getArguments() { return &arguments; }\n",
        body.replace("call->arguments", "*call->getArguments()")),
    "callee-helper": ("",
        body.replace("call->callee", "getCallee(call)")),
    "args-helper": ("",
        body.replace("call->arguments", "getArguments(call)")),
}
top = []
for name, (member, implementation) in cases.items():
    candidate = source.replace(header, header + member).replace(body, implementation)
    if name == "callee-helper":
        candidate = candidate.replace("void Print(ASTFunCall *call, std::ostream &output)",
            "inline ASTLValExpr *getCallee(ASTFunCall *call) { return call->callee; }\n"
            "void Print(ASTFunCall *call, std::ostream &output)")
    if name == "args-helper":
        candidate = candidate.replace("void Print(ASTFunCall *call, std::ostream &output)",
            "inline ASTNodeList<ASTExpr *> &getArguments(ASTFunCall *call) { return call->arguments; }\n"
            "void Print(ASTFunCall *call, std::ostream &output)")
    cpp = TMP / (name + ".cpp")
    cpp.write_text(candidate.replace('#include "../src/ASTNodeList.h"',
        '#include "' + str(ROOT / "vuc/src/ASTNodeList.h") + '"'))
    for flag, opts in {"o2":["-O2"], "rename":["-O2","-frename-registers"]}.items():
        obj = TMP / (name + "-" + flag + ".o")
        run = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",
                              *opts, "-c", str(cpp), "-o", str(obj)],
                             capture_output=True, text=True)
        if run.returncode:
            print("ERROR", name, flag, run.stderr[-120:], flush=True)
            continue
        try:
            result = match.run(obj, ROOT / "vuc/config/print_ast_fun_context_oct14_probe.json")
            record = (result["exact_bytes"], result["length"], name, flag,
                      result["different_offsets"][:10])
            top.append(record)
            if result["exact"]:
                print("NEW_EXACT", cpp, flag, flush=True)
                raise SystemExit(0)
        except ValueError as e:
            print("REJECT", name, flag, str(e)[:130], flush=True)
    print("CHECKED", name, flush=True)
for record in sorted(top, reverse=True):
    print("SCORE", record)

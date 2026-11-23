#!/usr/bin/env python3
"""Probe VCL-style static-inline helper and genuine Printer.cpp TU ownership.

GCC 3.3's native register allocator chooses EAX instead of original ECX for
ASTFunCall::callee at two byte offsets (+23, +29). VCL sometimes recovered
register allocation with genuine static inline wrappers, or by separating /
joining source owners. Test that hypothesis without changing accepted VUC
sources or hand-selecting registers.

Only original-length, fully relocation- and literal-validated results count.
"""
from pathlib import Path
import importlib.util
import itertools
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-printer-inline-owner-20261123")
OUT.mkdir(exist_ok=True)
base = (VUC / "src/Printer.cpp").read_text()
base = base.replace('#include "../src/ASTNodeList.h"',
                    '#include "' + str(VUC / "src/ASTNodeList.h") + '"')
mark = "void Print(ASTFunCall *call, std::ostream &output)"
assert base.count(mark) == 1
start = base.index(mark)
end = base.index("void Printer_UglyPrint(", start)
head, body, tail = base[:start], base[start:end], base[end:]
oldcallee = "Print(call->callee, output);"
oldargs = 'Print(call->arguments, output, ", ");'
assert oldcallee in body and oldargs in body

variants = {
    "baseline": ("", body),
    "callee-member-getter": (
        "", body.replace(oldcallee, "Print(call->GetCallee(), output);")),
    "callee-inline-getter": (
        "static inline ASTLValExpr *getCallee(ASTFunCall *call) { return call->callee; }\n",
        body.replace(oldcallee, "Print(getCallee(call), output);")),
    "callee-static-call-on-call": (
        "static inline void printCallee(ASTFunCall *call, std::ostream &out) "
        "{ Print(call->callee, out); }\n",
        body.replace(oldcallee, "printCallee(call, output);")),
    "callee-static-call-on-callee": (
        "static inline void printCallee(ASTLValExpr *expr, std::ostream &out) "
        "{ Print(expr, out); }\n",
        body.replace(oldcallee, "printCallee(call->callee, output);")),
    "callee-inline-ref": (
        "static inline void printCallee(ASTLValExpr *const &expr, std::ostream &out) "
        "{ Print(expr, out); }\n",
        body.replace(oldcallee, "printCallee(call->callee, output);")),
    "callee-inline-call-ref": (
        "static inline void printCallee(ASTFunCall *const &call, std::ostream &out) "
        "{ Print(call->callee, out); }\n",
        body.replace(oldcallee, "printCallee(call, output);")),
    "callee-template": (
        "template<typename T> static inline void printCallee(T *expr, std::ostream &out) "
        "{ Print(expr, out); }\n",
        body.replace(oldcallee, "printCallee(call->callee, output);")),
    "callee-template-ref": (
        "template<typename T> static inline void printCallee(T *const &expr, std::ostream &out) "
        "{ Print(expr, out); }\n",
        body.replace(oldcallee, "printCallee(call->callee, output);")),
    "callee-inline-const": (
        "inline void printCallee(ASTFunCall *const call, std::ostream &out) "
        "{ Print(call->callee, out); }\n",
        body.replace(oldcallee, "printCallee(call, output);")),
    "args-static-call-on-call": (
        "static inline void printArguments(ASTFunCall *call, std::ostream &out) "
        '{ Print(call->arguments, out, ", "); }\n',
        body.replace(oldargs, "printArguments(call, output);")),
    "args-static-call-on-args": (
        "static inline void printArguments(ASTNodeList<ASTExpr *> &list, std::ostream &out) "
        '{ Print(list, out, ", "); }\n',
        body.replace(oldargs, "printArguments(call->arguments, output);")),
    "args-static-template": (
        "template<typename T> static inline void printArguments(ASTNodeList<T> &list, "
        'std::ostream &out) { Print(list, out, ", "); }\n',
        body.replace(oldargs, "printArguments(call->arguments, output);")),
    "both-static-helpers": (
        "static inline void printCallee(ASTFunCall *call, std::ostream &out) "
        "{ Print(call->callee, out); }\n"
        "static inline void printArguments(ASTFunCall *call, std::ostream &out) "
        '{ Print(call->arguments, out, ", "); }\n',
        body.replace(oldcallee, "printCallee(call, output);").replace(
            oldargs, "printArguments(call, output);")),
    "full-body-static-inline": (
        "static inline void printFunCallBody(ASTFunCall *call, std::ostream &output)\n"
        + body[body.index("{"):] + "\n",
        mark + "\n{\n    printFunCallBody(call, output);\n}\n"),
    "full-body-template-inline": (
        "template<typename T> static inline void printFunCallBody(T *call, std::ostream &output)\n"
        + body[body.index("{"):] + "\n",
        mark + "\n{\n    printFunCallBody(call, output);\n}\n"),
}
# An inline class getter can change front-end value ownership.
getter = "    ASTLValExpr *GetCallee() { return callee; }\n"
layouts = {"member": head.replace("    ASTLValExpr *callee;\n",
                                   "    ASTLValExpr *callee;\n" + getter, 1),
           "ordinary": head}

spec = importlib.util.spec_from_file_location("match", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg = VUC / "config/print_ast_fun_context_oct14_probe.json"

rows = []
for (tag, (helpers, code)), owner, compiler in itertools.product(
        variants.items(), ("split", "joined"), ("jan25", "dec06")):
    prefix = layouts["member"] if tag == "callee-member-getter" else layouts["ordinary"]
    source = prefix + helpers + code + (tail if owner == "joined" else "")
    path = OUT / (tag + "-" + owner + ".cpp")
    path.write_text(source)
    obj = OUT / (tag + "-" + owner + "-" + compiler + ".o")
    args = ["/tmp/vuc-binutils215/g++-" + compiler + "-as215",
            "-O2", "-c", str(path), "-o", str(obj)]
    p = subprocess.run(args, capture_output=True, text=True)
    row = {"case": tag, "source_owner": owner, "compiler": compiler}
    if p.returncode:
        row["compile_failure"] = p.stderr[-400:]
    else:
        try:
            score = matcher.run(obj, cfg)
            row.update(size=score["length"], score=score["exact_bytes"],
                       diffs=score["different_offsets"], exact=score["exact"],
                       relocation_count=score["relocations"],
                       literal_bytes=score["verified_literal_bytes"])
        except (ValueError, KeyError) as error:
            row["unverified"] = str(error)
    rows.append(row)
    print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

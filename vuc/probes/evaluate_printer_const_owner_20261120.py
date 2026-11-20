#!/usr/bin/env python3
"""Test the VCL-proven top-level const parameter allocation mechanism in VUC.

The original ASTFunCall printer differs at only two callee-register bytes.
An i386 pointer-parameter top-level const is ABI/mangling neutral and may
change GCC global-register allocation without changing source semantics.
No assembler, dummy functions, or register constraints are used.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-printer-const-owner-20261120")
OUT.mkdir(exist_ok=True)
original = (VUC / "src/Printer.cpp").read_text()
original = original.replace(
    '#include "../src/ASTNodeList.h"',
    '#include "' + str(VUC / "src/ASTNodeList.h") + '"',
)
signature = "void Print(ASTFunCall *call, std::ostream &output)"
field = "    ASTLValExpr *callee;"
assert original.count(signature) == 1 and original.count(field) == 1
forms = {
    "baseline": original,
    "const_parameter": original.replace(
        signature, "void Print(ASTFunCall *const call, std::ostream &output)"
    ),
    "restrict_parameter": original.replace(
        signature,
        "void Print(ASTFunCall *__restrict__ call, std::ostream &output)",
    ),
    "const_restrict_parameter": original.replace(
        signature,
        "void Print(ASTFunCall *const __restrict__ call, std::ostream &output)",
    ),
    "const_member_pointer": original.replace(
        field, "    ASTLValExpr *const callee;"
    ),
    "const_param_and_member": original.replace(
        signature, "void Print(ASTFunCall *const call, std::ostream &output)"
    ).replace(field, "    ASTLValExpr *const callee;"),
}
spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg = VUC / "config/print_ast_fun_context_oct14_probe.json"
rows = []
for name, code in forms.items():
    cpp = OUT / (name + ".cpp")
    cpp.write_text(code)
    for snap in ("jan25", "dec06"):
        row = {"variant": name, "compiler": snap}
        obj = OUT / (name + "-" + snap + ".o")
        command = [
            "/tmp/vuc-binutils215/g++-" + snap + "-as215",
            "-O2", "-c", str(cpp), "-o", str(obj),
        ]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            row["error"] = result.stderr[-420:]
        else:
            try:
                score = matcher.run(obj, cfg)
                row["size"] = score["length"]
                row["matched"] = score["exact_bytes"]
                row["relocations"] = score["relocations"]
                row["literals"] = score["verified_literal_bytes"]
                row["diffs"] = score["different_offsets"]
                row["exact"] = score["exact"]
            except ValueError as exc:
                row["score_error"] = str(exc)
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

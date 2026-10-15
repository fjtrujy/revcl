#!/usr/bin/env python3
"""Test original Printer.cpp neighbouring members within one historical TU.

The original ELF has Print(ASTFunCall), four large AST printers,
Printer_UglyPrint, a global initializer, and four list template bodies.
Only *genuine* accepted bodies are composed here: the function-call
printer, module printer, and AST list printers. No fake predecessor or
injected register states are used.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/printer-true-tunit-oct15i")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
original = (VUC / "src/Printer.cpp").read_text().replace(
    '#include "../src/ASTNodeList.h"',
    '#include "' + str(VUC / "src/ASTNodeList.h") + '"')
header, fun_and_module = original.split("void Print(ASTFunCall *call, std::ostream &output)", 1)
fun = "void Print(ASTFunCall *call, std::ostream &output)" + fun_and_module.split("void Printer_UglyPrint(", 1)[0]
module = "void Printer_UglyPrint(" + fun_and_module.split("void Printer_UglyPrint(", 1)[1]
functions = (VUC / "src/PrintASTLists.cpp").read_text()
generic = functions[functions.index("template <class T>\nvoid Print("):]
# Keep only the authentic *definition* so explicit instantiations can appear
# after both application methods in their original symbol order.
gen_body, gen_inst = generic.split("template void Print(", 1)
gen_inst = "template void Print(" + gen_inst
variable_src = (VUC / "src/PrintASTVarList.cpp").read_text()
var_start = variable_src.index("template <class T>\nvoid Print(")
var_body = variable_src[var_start:variable_src.index("template void Print(", var_start)]
# Original ASTVarDecl list instantiation has a distinct expression ownership
# from the other three; model it as an ordinary C++03 specialization.
var_special = var_body.replace("template <class T>\nvoid Print(ASTNodeList<T> &",
    "template<>\nvoid Print(ASTNodeList<ASTVarDecl *> &").replace(
    "        T item = list.entries[i];",
    "        ASTVarDecl *item = list.entries[i];")
before = ("void Print(ASTExpr *, std::ostream &);\n"
          "void Print(ASTStmt *, std::ostream &);\n"
          "void Print(ASTDecl *, std::ostream &);\n"
          "void Print(ASTVarDecl *, std::ostream &);\n")
# Print ASTVarDecl through inherited ASTDecl: the original source's caller
# may rely on implicit conversion; overload declaration is intentionally
# omitted from the candidates so it doesn't create an unresolved overload.
before = before.replace("void Print(ASTVarDecl *, std::ostream &);\n", "")
# Variable-specialized template requires declaration but can be placed after
# generic definition; explicit instantiation would be its own source unit if
# incompatible with compiler emission in the combined printer TU.
varspecial = var_special + "\ntemplate void Print(ASTNodeList<ASTVarDecl *> &, std::ostream &, const char *);\n"
variants = {
    "base": original,
    "list-after": header + before + fun + module + gen_body + gen_inst,
    "list-before": header + before + gen_body + fun + module + gen_inst,
    "list-middle": header + before + fun + gen_body + module + gen_inst,
    "list-after-special": header + before + fun + module + gen_body + gen_inst + varspecial,
}
configs = {
    "call": "print_ast_fun_context_oct14_probe",
    "ugly": "ugly_printer_unit_oct13_probe",
    "startup": "iostream_global_printer_oct13_probe",
    "static": "iostream_static_0805b720_probe",
    "list_expr": "print_list_expr_exact_probe",
    "list_stmt": "print_list_stmt_exact_probe",
    "list_decl": "print_list_decl_exact_probe",
    "list_var": "print_list_var_exact_probe",
}
flags = {
    "original_plain": ["-O2"],
    "real_startup": ["-O2", "-frename-registers", "-fuse-cxa-atexit", "-ffunction-sections"],
}
results = []
for variant, source in variants.items():
    cpp = OUT / (variant + ".cpp")
    cpp.write_text(source)
    for name, opts in flags.items():
        obj = OUT / (variant + "-" + name + ".o")
        compiled = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts,
                                   "-c", str(cpp), "-o", str(obj)],
                                  capture_output=True, text=True)
        if compiled.returncode:
            row = {"variant": variant, "flags": name, "error": compiled.stderr[-320:]}
            results.append(row)
            print("COMPILE_ERROR", variant, name, compiled.stderr[-220:], flush=True)
            continue
        row = {"variant": variant, "flags": name, "functions": {}}
        for label, cfg in configs.items():
            try:
                matched = matcher.run(obj, VUC / "config" / (cfg + ".json"))
                row["functions"][label] = {
                    "match": matched["exact_bytes"], "size": matched["length"],
                    "exact": matched["exact"], "relocations": matched["relocations"],
                    "literals": matched["verified_literal_bytes"],
                    "diff_offsets": matched["different_offsets"][:12],
                }
            except ValueError as error:
                row["functions"][label] = {"error": str(error)}
        results.append(row)
        print(variant, name, {
            x: (v["match"], v["size"]) for x,v in row["functions"].items() if "match" in v
        }, flush=True)
(OUT / "results.json").write_text(json.dumps(results, indent=2) + "\n")

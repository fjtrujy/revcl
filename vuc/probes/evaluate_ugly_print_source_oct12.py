#!/usr/bin/env python3
"""Test original-era C++ member-access and literal lifetime forms for UglyPrint."""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/ugly-source-oct12")
TMP.mkdir(parents=True, exist_ok=True)
SPEC = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
MATCHER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MATCHER)
CONFIG = ROOT / "vuc/config/ugly_print_probe.json"
PFX = '''#include <ostream>
struct ASTDecl;
template <class T> struct ASTNodeList { void *begin_, *end_; };
struct ASTModule {
    char reserved_[0x20];
    ASTNodeList<ASTDecl *> declarations_;
    ASTNodeList<ASTDecl *> &GetDeclarations() { return declarations_; }
};
template <class T> void Print(ASTNodeList<T> &, std::ostream &, const char *);
'''
BODIES = {
    "baseline": 'Print(module->declarations_, output, "\\n");',
    "alias-first": 'ASTNodeList<ASTDecl *> &decls = module->declarations_; Print(decls, output, "\\n");',
    "string-first": 'const char *separator = "\\n"; Print(module->declarations_, output, separator);',
    "both-decls-first": 'ASTNodeList<ASTDecl *> &decls = module->declarations_; const char *separator = "\\n"; Print(decls, output, separator);',
    "both-string-first": 'const char *separator = "\\n"; ASTNodeList<ASTDecl *> &decls = module->declarations_; Print(decls, output, separator);',
    "getter": 'Print(module->GetDeclarations(), output, "\\n");',
    "getter-local": 'ASTNodeList<ASTDecl *> &decls = module->GetDeclarations(); Print(decls, output, "\\n");',
    "output-local": 'std::ostream &stream = output; Print(module->declarations_, stream, "\\n");',
    "output-local-first": 'std::ostream &stream = output; const char *separator = "\\n"; Print(module->declarations_, stream, separator);',
    "output-local-last": 'const char *separator = "\\n"; std::ostream &stream = output; Print(module->declarations_, stream, separator);',
    "module-local": 'ASTModule *original = module; Print(original->declarations_, output, "\\n");',
    "member-pointer": 'ASTNodeList<ASTDecl *> *decls = &module->declarations_; Print(*decls, output, "\\n");',
}
FLAGS = {
    "ren": ["-O2", "-frename-registers"],
    "plain": ["-O2"],
    "ren-j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "ren-nosched": ["-O2", "-frename-registers", "-fno-schedule-insns2"],
    "ren-nogcse": ["-O2", "-frename-registers", "-fno-gcse"],
    "ren-nopeephole": ["-O2", "-frename-registers", "-fno-peephole2"],
}
results=[]
for name, body in BODIES.items():
    src = TMP / (name + ".cpp")
    src.write_text(PFX + "\nvoid Printer_UglyPrint(ASTModule *module, std::ostream &output) { " + body + " }\n")
    for flagname, opts in FLAGS.items():
        obj = TMP / (name + "-" + flagname + ".o")
        cp = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(src), "-o", str(obj)], capture_output=True, text=True)
        if cp.returncode:
            results.append((0, name, flagname, cp.stderr[-90:]))
            continue
        try:
            r = MATCHER.run(obj, CONFIG)
            result = (r["exact_bytes"], name, flagname, r["length"], r["different_offsets"])
            results.append(result)
            if r["exact"]:
                print("EXACT", result)
                raise SystemExit(0)
        except ValueError as exc:
            results.append((0, name, flagname, str(exc)[:75]))
    print("checked", name, flush=True)
for record in sorted(results, reverse=True)[:30]:
    print(*record)

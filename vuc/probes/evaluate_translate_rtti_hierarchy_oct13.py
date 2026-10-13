#!/usr/bin/env python3
"""Recover AST statement-list translation with the authenticated IR RTTI hierarchy.

IR::Seq_Data derives from IR::Stmt_Data (not a synthetic shared Data base).
Compare genuine polymorphic base-class constructors, pointer membership and
natural list traversal; no raw assembly or patched relocation addresses.
"""
from pathlib import Path
import importlib.util
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/translate-rtti-hierarchy-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
original = (ROOT / "vuc/probes/translate_stmt_list.cpp").read_text().replace(
    '"../src/ASTNodeList.h"', '"' + str(ROOT / "vuc/src/ASTNodeList.h") + '"')
old = original[original.index("class Data {"):original.index("}\nIR::Stmt_Data *Translate")]
shapes = {
    "virtual-dtor": """class Stmt_Data {
public:
    virtual ~Stmt_Data();
    int kind;
    Stmt_Data() : kind(1) {}
};
class Seq_Data : public Stmt_Data {
public:
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Stmt_Data(), first(a), second(b) {}
};
""",
    "virtual-method": """class Stmt_Data {
public:
    virtual void method();
    int kind;
    Stmt_Data() : kind(1) {}
};
class Seq_Data : public Stmt_Data {
public:
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Stmt_Data(), first(a), second(b) {}
};
""",
    "virtual-dtor-and-override": """class Stmt_Data {
public:
    virtual ~Stmt_Data();
    int kind;
    Stmt_Data() : kind(1) {}
};
class Seq_Data : public Stmt_Data {
public:
    virtual ~Seq_Data();
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : Stmt_Data(), first(a), second(b) {}
};
""",
    "virtual-body": """class Stmt_Data {
public:
    virtual ~Stmt_Data();
    int kind;
    Stmt_Data() { kind = 1; }
};
class Seq_Data : public Stmt_Data {
public:
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) {
        first = a;
        second = b;
    }
};
""",
    "virtual-body-reverse": """class Stmt_Data {
public:
    virtual ~Stmt_Data();
    int kind;
    Stmt_Data() { kind = 1; }
};
class Seq_Data : public Stmt_Data {
public:
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) {
        second = b;
        first = a;
    }
};
""",
    "virtual-derived-initializers": """class Stmt_Data {
public:
    virtual ~Stmt_Data();
    int kind;
    Stmt_Data() : kind(1) {}
};
class Seq_Data : public Stmt_Data {
public:
    Stmt_Data *first;
    Stmt_Data *second;
    Seq_Data(Stmt_Data *a, Stmt_Data *b) : second(b), first(a) {}
};
""",
}
loop_text = original[original.index("IR::Stmt_Data *Translate(ASTNodeList"):]
loop_variants = {
    "original": loop_text,
    "plain-conversion": loop_text.replace("(IR::Stmt_Data *)new IR::Seq_Data", "new IR::Seq_Data"),
    "early-continue": loop_text.replace(
        """        if (item) {
            if (result)
                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);
            else
                result=item;
        }""",
        """        if (!item) continue;
        if (!result) result = item;
        else result = new IR::Seq_Data(result, item);"""),
}
flags = {
    "ren": ["-O2", "-frename-registers"],
    "plain": ["-O2"],
    "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
}
rank = []
for hierarchy, class_decl in shapes.items():
    for loop, function in loop_variants.items():
        src = SCRATCH / (hierarchy + "-" + loop + ".cpp")
        src.write_text(original.replace(old, class_decl).replace(loop_text, function))
        for key, opts in flags.items():
            obj = SCRATCH / (hierarchy + "-" + loop + "-" + key + ".o")
            build = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(src), "-o", str(obj)], text=True, capture_output=True)
            if build.returncode:
                print("COMPILE_FAIL", hierarchy, loop, key, build.stderr[-140:], flush=True)
                continue
            symbols = subprocess.check_output(["nm", "-S", str(obj)], text=True).splitlines()
            entry = next((line for line in symbols if line.endswith(" _Z9TranslateR11ASTNodeListIP7ASTStmtE")), None)
            length = int(entry.split()[1], 16) if entry else -1
            exact_bytes = 0
            if length == 119:
                try:
                    result = matcher.run(obj, ROOT / "vuc/config/translate_stmt_list_probe.json")
                    exact_bytes = result["exact_bytes"]
                    if result["exact"]:
                        print("EXACT", hierarchy, loop, key, src, flush=True)
                        raise SystemExit(0)
                except ValueError as error:
                    print("MATCH_FAIL", hierarchy, loop, key, str(error)[:150], flush=True)
            rank.append((exact_bytes, -abs(119 - length), length, hierarchy, loop, key))
            if exact_bytes:
                print("SAME_LENGTH", exact_bytes, length, hierarchy, loop, key, flush=True)
        print("checked", hierarchy, loop, flush=True)
print("TOP")
for item in sorted(rank, reverse=True)[:32]:
    print(item)

#!/usr/bin/env python3
"""Historical GCC tests of authentic AST statement-list translation and Seq_Data construction.

Reconstruct the original sequence node as a polymorphic class with a Data
base and two statement pointers. Change real C++ initialization/loop forms,
never bytes or link addresses; original relocation targets remain authenticated.
"""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/translate-seq-shapes-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
original = (ROOT / "vuc/probes/translate_stmt_list.cpp").read_text().replace(
    '"../src/ASTNodeList.h"', '"' + str(ROOT / "vuc/src/ASTNodeList.h") + '"')
old_ctor = 'Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), first(a), second(b) {}'
assert old_ctor in original
ctors = {
    "ctor-initializers": old_ctor,
    "ctor-body-first": 'Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data() { first = a; second = b; }',
    "ctor-body-second": 'Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data() { second = b; first = a; }',
    "ctor-initializers-reversed": 'Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), second(b), first(a) {}',
    "ctor-body-mixed": 'Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), first(a) { second = b; }',
    "ctor-body-mixed2": 'Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data(), second(b) { first = a; }',
    "ctor-additional-alias": 'Seq_Data(Stmt_Data *a, Stmt_Data *b) : Data() { Stmt_Data *lhs = a; first = lhs; second = b; }',
}
old_loop = '''    IR::Stmt_Data *result = 0;
    for (int i=0; i<statements.length; ++i) {
        IR::Stmt_Data *item=Translate(statements.entries[i]);
        if (item) {
            if (result)
                result=(IR::Stmt_Data *)new IR::Seq_Data(result, item);
            else
                result=item;
        }
    }
    return result;'''
assert old_loop in original
loops = {
    "baseline": old_loop,
    "continue-first": '''    IR::Stmt_Data *result = 0;
    for (int i = 0; i < statements.length; ++i) {
        IR::Stmt_Data *item = Translate(statements.entries[i]);
        if (!item) continue;
        if (!result) result = item;
        else result = (IR::Stmt_Data *)new IR::Seq_Data(result, item);
    }
    return result;''',
    "invert-first": '''    IR::Stmt_Data *result = 0;
    for (int i = 0; i < statements.length; ++i) {
        IR::Stmt_Data *item = Translate(statements.entries[i]);
        if (item) {
            if (!result) result = item;
            else result = (IR::Stmt_Data *)new IR::Seq_Data(result, item);
        }
    }
    return result;''',
    "item-first": '''    IR::Stmt_Data *result = 0;
    for (int i = 0; i < statements.length; ++i) {
        IR::Stmt_Data *item = Translate(statements.entries[i]);
        if (item && result) {
            result = (IR::Stmt_Data *)new IR::Seq_Data(result, item);
        } else if (item) {
            result = item;
        }
    }
    return result;''',
    "result-local-cast": '''    IR::Stmt_Data *result = 0;
    for (int i = 0; i < statements.length; ++i) {
        IR::Stmt_Data *item = Translate(statements.entries[i]);
        if (!item) continue;
        IR::Stmt_Data *previous = result;
        if (previous) {
            IR::Seq_Data *sequence = new IR::Seq_Data(previous, item);
            result = (IR::Stmt_Data *)sequence;
        } else result = item;
    }
    return result;''',
}
flags = {
    "baseline": ["-O2", "-frename-registers"],
    "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "plain": ["-O2"],
}
config = ROOT / "vuc/config/translate_stmt_list_probe.json"
rank = []
for constructor, ctor in ctors.items():
    for loop, code in loops.items():
        src = SCRATCH / (constructor + "-" + loop + ".cpp")
        src.write_text(original.replace(old_ctor, ctor).replace(old_loop, code))
        for label, args in flags.items():
            obj = SCRATCH / (constructor + "-" + loop + "-" + label + ".o")
            build = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *args, "-c", str(src), "-o", str(obj)], capture_output=True, text=True)
            if build.returncode:
                print("COMPILE_FAIL", constructor, loop, label, build.stderr[-100:], flush=True)
                continue
            entry = next((row for row in subprocess.check_output(["nm", "-S", str(obj)], text=True).splitlines()
                         if row.endswith(" _Z9TranslateR11ASTNodeListIP7ASTStmtE")), None)
            size = int(entry.split()[1], 16) if entry else -1
            matched = None
            if size == 119:
                try:
                    r = matcher.run(obj, config)
                    matched = r["exact_bytes"]
                    if r["exact"]:
                        print("EXACT", constructor, loop, label, src, flush=True)
                        raise SystemExit(0)
                except ValueError as exc:
                    print("REJECT", constructor, loop, label, str(exc)[:100], flush=True)
            rank.append((matched or 0, -abs(119-size), size, constructor, loop, label))
        print("tested", constructor, loop, flush=True)
print("TOP")
for row in sorted(rank, reverse=True)[:35]:
    print(row)

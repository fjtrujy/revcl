#!/usr/bin/env python3
"""VUC Instruction arena: genuine global definition order and GCC scratch state.

The original 31-byte startup wrapper is named
_GLOBAL__I__ZN11Instruction9instrDataE, whereas the previous combined
source-unit scratch output was keyed to InstructionList::Add.
Moving the actual Instruction::instrData definition before genuine
functions tests original symbol ownership, not made-up compiler padding.
Protect the original exact 265-byte vector and 16-byte __tcf_0.
"""
from pathlib import Path
import json
import importlib.util
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-instruction-global-owner-20261120")
OUT.mkdir(exist_ok=True)
accepted = (VUC / "src/InstructionListLineCommentUnit.cpp").read_text()
arena = (VUC / "probes/instruction_full_source_unit_oct13.cpp").read_text()
accepted = accepted.replace(
    '#include "MLGameString.h"',
    '#include "' + str(VUC / "src/MLGameString.h") + '"',
)
accepted = "#include <iostream>\n" + accepted
assert accepted.count("    static void *operator new(std::size_t);") == 1
accepted = accepted.replace(
    "    static void *operator new(std::size_t);",
    "    static char *instrData;\n    static void *operator new(std::size_t);",
    1,
)
suffix = arena[arena.index("char *Instruction::instrData;"):]
assert suffix.count("char *Instruction::instrData;") == 1
assert "return text_.c_str();" in suffix
suffix = suffix.replace("return text_.c_str();", "return text.c_str();", 1)
base = accepted + "\n" + suffix
definition = "char *Instruction::instrData;\n"
assert base.count(definition) == 1
base = base.replace(definition, "", 1)
anchors = {
    "original-late": None,
    "first-strong": "void InstructionList::Add(Instruction *instruction)",
    "first-strong-then-vector": "COMPOSE-GLOBAL-VECTOR",
    "vector-then-first-strong": "COMPOSE-VECTOR-GLOBAL",
    "pre-print": "void InstructionList::Print(std::ostream &output)",
    "pre-addfmt": "void InstructionList::Add(const char *format, ...)",
    "pre-line": "void InstructionList::AddLineComment(int line)",
    "pre-instruction-method": "const char *Instruction::ToString()",
    "pre-arena": "struct InstructionArenaCleanup {",
}
instantiation = """
template void std::vector<Instruction *>::_M_insert_aux(
    std::vector<Instruction *>::iterator, Instruction *const &);
"""
spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
tests = {
    "cleanup": "instruction_arena_cleanup_unit_oct14_probe.json",
    "vector": "instruction_vector_insert_exact_oct15i.json",
    "iostream_tcf0": "iostream_cleanup_08051970_probe.json",
    "startup": "instruction_arena_global_probe.json",
}
rows = []
for tag, at in anchors.items():
    if at is None:
        code = base + "\n" + definition
    elif at in ("COMPOSE-GLOBAL-VECTOR", "COMPOSE-VECTOR-GLOBAL"):
        first_method = "void InstructionList::Add(Instruction *instruction)"
        assert first_method in base
        ordered = (definition + instantiation if at == "COMPOSE-GLOBAL-VECTOR"
                   else instantiation + definition)
        code = base.replace(first_method, ordered + first_method, 1)
    else:
        assert at in base, at
        code = base.replace(at, definition + at, 1)
    cpp = OUT / (tag + ".cpp")
    cpp.write_text(code)
    for snap in ("jan25", "dec06"):
        row = {"case": tag, "compiler": snap}
        obj = OUT / (tag + "-" + snap + ".o")
        p = subprocess.run(
            ["/tmp/vuc-binutils215/g++-" + snap + "-as215",
             "-O2", "-frename-registers", "-falign-jumps=1",
             "-fuse-cxa-atexit", "-c", str(cpp), "-o", str(obj)],
            capture_output=True, text=True,
        )
        if p.returncode:
            row["error"] = p.stderr[-400:]
        else:
            for name, cfg in tests.items():
                try:
                    score = m.run(obj, VUC / "config" / cfg)
                    row[name] = {
                        "matched": score["exact_bytes"],
                        "size": score["length"],
                        "exact": score["exact"],
                        "diffs": score["different_offsets"][:13],
                    }
                except ValueError as exc:
                    row[name] = {"error": str(exc)}
            dis = subprocess.run(
                ["objdump", "-d", "-Mintel", str(obj)],
                capture_output=True, text=True,
            ).stdout
            if "<__tcf_1>:" in dis:
                part = dis[dis.index("<__tcf_1>:"):]
                part = part[:part.find("\n\n")]
                row["pop_regs"] = [
                    l.split()[-1] for l in part.splitlines()
                    if "\tpop" in l
                ][-3:]
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

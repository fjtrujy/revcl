#!/usr/bin/env python3
"""Test VCL-style static inline helpers and genuine source-unit joins for VUC.

Target: Instruction arena's original 46-byte __tcf_1 cleanup, whose only
remaining difference is a dead pop-register byte. Reconstruct real C++03
helper contracts and compare both a minimal Instruction owner and the
coexisting, original-exact Instruction* vector owner. Never force registers,
insert assembler/padding, or modify accepted source files.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-arena-inline-owner-20261123")
OUT.mkdir(exist_ok=True)
simple = (VUC / "probes/instruction_full_source_unit_oct13.cpp").read_text()
simple = simple.replace('#include "../src/MLGameString.h"',
                        '#include "' + str(VUC / "src/MLGameString.h") + '"')
start = simple.index("    ~InstructionArenaCleanup() {")
end = simple.index("\n};\nstatic InstructionArenaCleanup cleanup;", start)
old_dtor = simple[start:end]
assert "while (Instruction::instrData)" in old_dtor

owner = (VUC / "src/InstructionListLineCommentUnit.cpp").read_text()
owner = owner.replace('#include "MLGameString.h"',
                      '#include "' + str(VUC / "src/MLGameString.h") + '"')
owner = "#include <iostream>\n#include <new>\n" + owner
owner = owner.replace("    static void *operator new(std::size_t);",
                      "    static char *instrData;\n    static void *operator new(std::size_t);")
real = simple[simple.index("char *Instruction::instrData;"):]
real = real.replace("return text_.c_str();", "return text.c_str();", 1)

helpers = {
    "baseline": ("", old_dtor),
    "inline-entire-dtor": (
        """static inline void releaseAllInstructionChunks() {
    while (Instruction::instrData) {
        char *current = Instruction::instrData;
        char *next = *(char **)(current + 0x2004);
        ::operator delete(current);
        Instruction::instrData = next;
    }
}
""",
        "    ~InstructionArenaCleanup() { releaseAllInstructionChunks(); }"),
    "inline-reference-dtor": (
        """static inline void releaseAllInstructionChunks(char *&head) {
    while (head) {
        char *current = head;
        char *next = *(char **)(current + 0x2004);
        ::operator delete(current);
        head = next;
    }
}
""",
        "    ~InstructionArenaCleanup() { releaseAllInstructionChunks(Instruction::instrData); }"),
    "inline-head-query": (
        """static inline bool hasInstructionChunks() {
    return Instruction::instrData != 0;
}
""",
        old_dtor.replace("while (Instruction::instrData)", "while (hasInstructionChunks())")),
    "inline-head-getter": (
        """static inline char *&getInstructionHead() {
    return Instruction::instrData;
}
""",
        old_dtor.replace("Instruction::instrData", "getInstructionHead()")),
    "inline-next-chunk": (
        """static inline char *nextInstructionChunk(char *chunk) {
    return *(char **)(chunk + 0x2004);
}
""",
        old_dtor.replace("*(char **)(current + 0x2004)", "nextInstructionChunk(current)")),
    "inline-delete": (
        """static inline void deleteInstructionChunk(char *chunk) {
    ::operator delete(chunk);
}
""",
        old_dtor.replace("::operator delete(current);", "deleteInstructionChunk(current);")),
    "inline-chunk-step": (
        """static inline void releaseOneInstructionChunk() {
    char *current = Instruction::instrData;
    char *next = *(char **)(current + 0x2004);
    ::operator delete(current);
    Instruction::instrData = next;
}
""",
        """    ~InstructionArenaCleanup() {
        while (Instruction::instrData)
            releaseOneInstructionChunk();
    }"""),
    "inline-next-and-delete": (
        """static inline char *releaseInstructionChunk(char *current) {
    char *next = *(char **)(current + 0x2004);
    ::operator delete(current);
    return next;
}
""",
        """    ~InstructionArenaCleanup() {
        while (Instruction::instrData)
            Instruction::instrData = releaseInstructionChunk(Instruction::instrData);
    }"""),
}

spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg_cleanup = VUC / "config/instruction_arena_cleanup_unit_oct14_probe.json"
cfg_vector = VUC / "config/instruction_vector_insert_exact_oct15i.json"
instantiation = """
template void std::vector<Instruction *>::_M_insert_aux(
    std::vector<Instruction *>::iterator, Instruction *const &);
"""
rows = []
for name, (helper, destructor) in helpers.items():
    for context in ("minimal", "joined", "joined-vector-before-add"):
        if context == "minimal":
            source = simple
        else:
            source = owner + "\n" + real
            if context.endswith("before-add"):
                source = source.replace("void InstructionList::Add(Instruction *instruction)",
                                        instantiation + "\nvoid InstructionList::Add(Instruction *instruction)", 1)
        assert old_dtor in source
        source = source.replace("struct InstructionArenaCleanup {", helper + "\nstruct InstructionArenaCleanup {", 1)
        source = source.replace(old_dtor, destructor, 1)
        path = OUT / (name + "-" + context + ".cpp")
        path.write_text(source)
        for snap in ("jan25", "dec06"):
            obj = OUT / (name + "-" + context + "-" + snap + ".o")
            args = ["/tmp/vuc-binutils215/g++-" + snap + "-as215",
                    "-O2", "-frename-registers", "-fuse-cxa-atexit",
                    "-falign-jumps=1", "-c", str(path), "-o", str(obj)]
            proc = subprocess.run(args, capture_output=True, text=True)
            row = {"case": name, "owner": context, "compiler": snap}
            if proc.returncode:
                row["compile_failure"] = proc.stderr[-400:]
            else:
                for key, cfg in (("cleanup", cfg_cleanup), ("vector", cfg_vector)):
                    try:
                        measured = matcher.run(obj, cfg)
                        row[key] = {"size": measured["length"],
                                    "matched": measured["exact_bytes"],
                                    "diffs": measured["different_offsets"],
                                    "exact": measured["exact"],
                                    "relocations": measured["relocations"]}
                    except (ValueError, KeyError) as exc:
                        row[key] = {"unverified": str(exc)}
            print(json.dumps(row, sort_keys=True), flush=True)
            rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

#!/usr/bin/env python3
"""Co-instantiate original-exact Instruction* vector with real arena cleanup.

VCL's strongest dead-pop recoveries used actual template predecessors, not
arbitrary changes to the cleanup function. This experiment uses the canonical
C++03 source that independently matches the 265-byte vector insert and
appends the real arena methods plus cleanup object. All bodies are source
reconstructions; no inline ASM, pads or invented addresses.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-instruction-exact-vector-owner-20261120")
OUT.mkdir(exist_ok=True)
accepted = (VUC / "src/InstructionListLineCommentUnit.cpp").read_text()
src_arena = (VUC / "probes/instruction_full_source_unit_oct13.cpp").read_text()
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
end = src_arena.index("char *Instruction::instrData;")
real = src_arena[end:]
assert "return text_.c_str();" in real
real = real.replace("return text_.c_str();", "return text.c_str();", 1)
instantiation = """
template void std::vector<Instruction *>::_M_insert_aux(
    std::vector<Instruction *>::iterator, Instruction *const &);
"""
# The canonical InstructionListLineCommentUnit already defines the exact
# vector owner; add the real, unchanged Instruction class methods and arena.
cases = {
    "append-arena": accepted + "\n" + real,
    "arena-before-variadics": accepted.replace(
        "void InstructionList::Add(const char *format, ...)",
        real + "\nvoid InstructionList::Add(const char *format, ...)",
        1,
    ),
    "arena-before-print": accepted.replace(
        "void InstructionList::Print(std::ostream &output)",
        real + "\nvoid InstructionList::Print(std::ostream &output)",
        1,
    ),
    "vector-before-add": accepted.replace(
        "void InstructionList::Add(Instruction *instruction)",
        instantiation + "\nvoid InstructionList::Add(Instruction *instruction)",
        1,
    ) + "\n" + real,
    "vector-before-print": accepted.replace(
        "void InstructionList::Print(std::ostream &output)",
        instantiation + "\nvoid InstructionList::Print(std::ostream &output)",
        1,
    ) + "\n" + real,
    "vector-before-arena": accepted + "\n" + real.replace(
        "struct InstructionArenaCleanup {",
        instantiation + "\nstruct InstructionArenaCleanup {",
        1,
    ),
    "vector-after-arena": accepted + "\n" + real + instantiation,
}
spec = importlib.util.spec_from_file_location("match", VUC / "scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
targets = {
    "cleanup": "instruction_arena_cleanup_unit_oct14_probe.json",
    "vector": "instruction_vector_insert_exact_oct15i.json",
}
flagsets = {
    "without-cxa": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "with-cxa": [
        "-O2", "-frename-registers", "-falign-jumps=1", "-fuse-cxa-atexit"
    ],
}
rows = []
for tag, code in cases.items():
    path = OUT / (tag + ".cpp")
    path.write_text(code)
    for flagtag, flags in flagsets.items():
      for snap in ("jan25", "dec06"):
        row = {"case": tag, "compiler": snap, "flagset": flagtag}
        obj = OUT / (tag + "-" + flagtag + "-" + snap + ".o")
        cp = subprocess.run(
            ["/tmp/vuc-binutils215/g++-" + snap + "-as215",
             *flags, "-c", str(path), "-o", str(obj)],
            capture_output=True, text=True,
        )
        if cp.returncode:
            row["compile_error"] = cp.stderr[-450:]
        else:
            for name, config in targets.items():
                try:
                    result = m.run(obj, VUC / "config" / config)
                    row[name] = {
                        "matched": result["exact_bytes"],
                        "size": result["length"],
                        "relocations": result["relocations"],
                        "diffs": result["different_offsets"][:20],
                        "exact": result["exact"],
                    }
                except ValueError as exc:
                    row[name] = {"error": str(exc)}
            dis = subprocess.run(
                ["objdump", "-d", "-Mintel", str(obj)],
                capture_output=True, text=True,
            ).stdout
            marker = "<__tcf_1>:"
            if marker in dis:
                body = dis[dis.index(marker):]
                body = body[:body.find("\n\n")]
                row["pop_registers"] = [
                    line.split()[-1] for line in body.splitlines() if "\tpop" in line
                ][-3:]
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

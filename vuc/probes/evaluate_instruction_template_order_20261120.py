#!/usr/bin/env python3
"""Instruction arena __tcf_1: genuine vector-template emission-order controls.

VCL exact successes show that a real weak template's peephole2 traversal
position can influence a later destructor's dead stack-discard register.
This reproduces the authentic InstructionList vector insertion, changing
only legitimate explicit C++03 instantiation placement.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-instruction-template-order-20261120")
OUT.mkdir(exist_ok=True)
source = (VUC / "probes/instruction_list_arena_unit_oct14.cpp").read_text()
source = source.replace(
    '#include "../src/MLGameString.h"',
    '#include "' + str(VUC / "src/MLGameString.h") + '"',
)
inst = """
// Explicit C++03 instantiation of the genuine Instruction* vector helper.
template void std::vector<Instruction *>::_M_insert_aux(
    std::vector<Instruction *>::iterator, Instruction *const &);
"""
anchors = {
    "baseline": None,
    "before_add": "void InstructionList::Add(Instruction *instruction)",
    "before_print": "void InstructionList::Print(std::ostream &output)",
    "before_new": "void *Instruction::operator new(std::size_t size)",
    "before_cleanup_type": "struct InstructionArenaCleanup {",
    "before_cleanup_instance": "static InstructionArenaCleanup cleanup;",
    "after_cleanup": "",
}
spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cleanup_cfg = VUC / "config/instruction_arena_cleanup_unit_oct14_probe.json"
vector_cfg = VUC / "config/instruction_vector_insert_exact_oct15i.json"
rows = []
for label, before in anchors.items():
    if before is None:
        code = source
    elif before == "":
        code = source + "\n" + inst
    else:
        assert before in source
        code = source.replace(before, inst + "\n" + before, 1)
    cpp = OUT / (label + ".cpp")
    cpp.write_text(code)
    for snap in ("jan25", "dec06"):
        row = {"case": label, "compiler": snap}
        obj = OUT / (label + "-" + snap + ".o")
        cp = subprocess.run([
            "/tmp/vuc-binutils215/g++-" + snap + "-as215",
            "-O2", "-frename-registers", "-fuse-cxa-atexit",
            "-c", str(cpp), "-o", str(obj),
        ], capture_output=True, text=True)
        if cp.returncode:
            row["compile_error"] = cp.stderr[-360:]
        else:
            for name, cfg in (("cleanup", cleanup_cfg), ("vector", vector_cfg)):
                try:
                    score = matcher.run(obj, cfg)
                    row[name] = {
                        "matched": score["exact_bytes"],
                        "size": score["length"],
                        "exact": score["exact"],
                        "diffs": score["different_offsets"][:15],
                    }
                except ValueError as exc:
                    row[name] = {"error": str(exc)}
            dis = subprocess.run(
                ["objdump", "-d", "-Mintel", str(obj)],
                capture_output=True, text=True,
            ).stdout
            marker = "<__tcf_1>:"
            if marker in dis:
                end = dis.index("\n\n", dis.index(marker))
                text = dis[dis.index(marker):end]
                row["pop_line"] = [
                    s.strip() for s in text.splitlines()
                    if "\tpop" in s
                ][-3:]
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

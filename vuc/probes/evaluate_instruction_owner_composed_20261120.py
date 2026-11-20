#!/usr/bin/env python3
"""VUC Instruction cleanup: combine genuine method order and vector owner.

The VCL exact reconstruction showed that one real deferred template
predecessor can rotate peephole2's global scratch cursor. This tests the
observed pop EBX -> EAX transition in a *complete* recovered Instruction
source-unit context, using only actual C++03 methods and one authentic
Instruction* vector specialization.
"""
from pathlib import Path
import importlib.util
import json
import re
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-instruction-owner-composed-20261120")
OUT.mkdir(exist_ok=True)
accepted = (VUC / "src/InstructionListLineCommentUnit.cpp").read_text()
instr = (VUC / "probes/instruction_full_source_unit_oct13.cpp").read_text()
start = accepted.index("void InstructionList::Add(Instruction *")
header = accepted[:start].replace(
    '#include "MLGameString.h"',
    '#include "' + str(VUC / "src/MLGameString.h") + '"',
)
header = '#include <iostream>\n' + header
header = header.replace("MLGameString text;", "MLGameString text_;")
header = header.replace(" : text(name),", " : text_(name),")
header = header.replace(
    "    static void *operator new(std::size_t);",
    "    static char *instrData;\n    static void *operator new(std::size_t);",
)
header = header.replace(
    "    void AddLineComment(int line);",
    "    void AddLineComment(int line);\n    void GenerateImage(unsigned char *);",
)
function_re = re.compile(r"(?m)^(?:void|MLGameString) InstructionList::")
locations = list(function_re.finditer(accepted))
methods = {}
for i, entry in enumerate(locations):
    text = accepted[entry.start():
                    locations[i + 1].start() if i + 1 < len(locations) else len(accepted)]
    text = text.strip()
    signature = text[:105]
    if "Add(Instruction *" in signature:
        key = "add"
    elif "Add(const char *" in signature:
        key = "addfmt"
    elif "AddB(" in signature:
        key = "addb"
    elif "AddL(" in signature:
        key = "addl"
    elif "AddLineComment(" in signature:
        key = "line"
    elif "GenLabel(" in signature:
        key = "label"
    elif "GenTmp(" in signature:
        key = "tmp"
    elif "Print(" in signature:
        key = "print"
    else:
        raise ValueError(signature)
    methods[key] = text
assert set(methods) == {"add", "addfmt", "addb", "addl", "line", "label", "tmp", "print"}
methods["image"] = "void InstructionList::GenerateImage(unsigned char *) {}"
suffix = (
    "char *Instruction::instrData;\n" +
    instr[instr.index("const char *Instruction::ToString()"):]
)
instantiation = """
template void std::vector<Instruction *>::_M_insert_aux(
    std::vector<Instruction *>::iterator, Instruction *const &);
"""
sequences = {
    "short": ["add", "addl", "label", "tmp", "print"],
    "real_first": ["add", "addfmt", "addb", "addl", "line"],
    "complete": ["add", "addfmt", "addb", "addl", "line",
                 "image", "label", "tmp", "print"],
}
placement = {
    "no_explicit": None,
    "before_add": "before_add",
    "before_print_or_suffix": "before_print_or_suffix",
    "before_cleanup": "before_cleanup",
    "after_cleanup": "after_cleanup",
}
spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
cfg = VUC / "config/instruction_arena_cleanup_unit_oct14_probe.json"
rows = []
for name, sequence in sequences.items():
    fragments = [methods[m] for m in sequence]
    for place, _ in placement.items():
        if place == "no_explicit":
            code = header + "\n".join(fragments) + "\n" + suffix
        elif place == "before_add":
            code = header + instantiation + "\n".join(fragments) + "\n" + suffix
        elif place == "before_print_or_suffix":
            if "print" in sequence:
                index = sequence.index("print")
                contents = fragments[:index] + [instantiation] + fragments[index:]
                code = header + "\n".join(contents) + "\n" + suffix
            else:
                code = header + "\n".join(fragments) + "\n" + instantiation + suffix
        elif place == "before_cleanup":
            code = header + "\n".join(fragments) + "\n" + suffix.replace(
                "struct InstructionArenaCleanup {",
                instantiation + "\nstruct InstructionArenaCleanup {",
                1,
            )
        else:
            code = header + "\n".join(fragments) + "\n" + suffix + instantiation
        label = name + "-" + place
        src = OUT / (label + ".cpp")
        src.write_text(code)
        for compiler in ("jan25", "dec06"):
            row = {"owner": name, "instantiation": place, "compiler": compiler}
            obj = OUT / (label + "-" + compiler + ".o")
            cp = subprocess.run([
                "/tmp/vuc-binutils215/g++-" + compiler + "-as215",
                "-O2", "-frename-registers", "-fuse-cxa-atexit",
                "-c", str(src), "-o", str(obj),
            ], capture_output=True, text=True)
            if cp.returncode:
                row["error"] = cp.stderr[-280:]
            else:
                try:
                    score = match.run(obj, cfg)
                    row["match"] = score["exact_bytes"]
                    row["size"] = score["length"]
                    row["diff"] = score["different_offsets"]
                    row["exact"] = score["exact"]
                    row["relocations"] = score["relocations"]
                except ValueError as exc:
                    row["score_error"] = str(exc)
            print(json.dumps(row, sort_keys=True), flush=True)
            rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

#!/usr/bin/env python3
"""Check a real-order Instruction.cpp TU against *all* recovered neighbours.

This is not a register-flag sweep: two evidence-based configurations compile
real reconstructed C++03 InstructionList/Instruction/arena member definitions
in their original ELF order, then compare each method independently.
"""
from pathlib import Path
import importlib.util
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/instruction-true-tunit-oct15i")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
unit = (VUC / "src/InstructionListLineCommentUnit.cpp").read_text()
arena = (VUC / "probes/instruction_full_source_unit_oct13.cpp").read_text()
ix = unit.index("void InstructionList::Add(Instruction *")
header = unit[:ix]
header = header.replace('#include "MLGameString.h"', '#include "' + str(VUC / "src/MLGameString.h") + '"')
header = "#include <iostream>\n#include <new>\n" + header
header = header.replace("    static void *operator new(std::size_t);",
                        "    static char *instrData;\n    static void *operator new(std::size_t);")
header = header.replace("    void AddLineComment(int line);",
                        "    void AddLineComment(int line);\n    void GenerateImage(unsigned char *);")
methods = {}
names = list(re.finditer(r"(?m)^(?:void|MLGameString) InstructionList::", unit))
for i, found in enumerate(names):
    body = unit[found.start(): names[i+1].start() if i+1 < len(names) else len(unit)].strip()
    if "Add(Instruction *" in body[:85]: name = "add"
    elif "Add(const char *" in body[:85]: name = "addformat"
    elif "AddB(" in body[:85]: name = "addb"
    elif "AddL(" in body[:85]: name = "addl"
    elif "AddLineComment(" in body[:85]: name = "line"
    elif "GenLabel(" in body[:85]: name = "label"
    elif "GenTmp(" in body[:85]: name = "temp"
    elif "Print(" in body[:85]: name = "print"
    else: raise ValueError(body[:95])
    methods[name] = body
assert len(methods) == 8, methods.keys()
methods["image"] = "void InstructionList::GenerateImage(unsigned char *) {}"
arena_body = arena[arena.index("const char *Instruction::ToString()"):].replace("text_", "text")
arena_body = arena_body.replace("char *Instruction::instrData;", "")
# The original source order is established by .text symbols. All ordinary
# methods, not an invented compiler-register predecessor, are included.
order = ("add", "addformat", "addb", "addl", "line", "image", "label", "temp", "print")
body = header + "\n" + "\n\n".join(methods[x] for x in order) + "\nchar *Instruction::instrData;\n" + arena_body
src = OUT / "InstructionOriginalOrder.cpp"
src.write_text(body)
configs = {
    "add": "instruction_list_add_exact_probe",
    "addformat": "instruction_add_format_probe",
    "addb": "instruction_addb_format_exact_oct13_probe",
    "addl": "instruction_addl_probe",
    "line": "instruction_add_line_comment_context_oct13_probe",
    "image": "noop_080517d0_probe",
    "label": "instruction_gen_label_probe",
    "temp": "instruction_gen_tmp_probe",
    "print": "instruction_list_print_probe",
    "tostring": "instruction_to_string_probe",
    "statinit": "instruction_arena_static_probe",
    "globinit": "instruction_arena_global_probe",
    "cleanup": "instruction_arena_cleanup_unit_oct14_probe",
}
# Baseline ABI options and single jump-alignment control justified by the
# multiple already accepted InstructionList members, not arbitrary flags.
opts = {
    "full-cxa-sections": ["-O2", "-frename-registers", "-fuse-cxa-atexit", "-ffunction-sections"],
    "j1-cxa-sections": ["-O2", "-frename-registers", "-fuse-cxa-atexit", "-ffunction-sections", "-falign-jumps=1"],
}
results = []
for snap in ("jan25", "dec06"):
    for label, flags in opts.items():
        obj = OUT / (snap + "-" + label + ".o")
        p = subprocess.run(["/tmp/vuc-binutils215/g++-" + snap + "-as215", *flags,
                            "-c", str(src), "-o", str(obj)], capture_output=True, text=True)
        if p.returncode:
            results.append({"compiler": snap, "configuration": label, "error": p.stderr[-600:]})
            print("COMPILE_FAIL", snap, label, p.stderr[-150:], flush=True)
            continue
        row = {"compiler": snap, "configuration": label, "functions": {}}
        for name, basename in configs.items():
            try:
                matched = matcher.run(obj, VUC / "config" / (basename + ".json"))
                row["functions"][name] = {
                    "size": matched["length"], "matched": matched["exact_bytes"],
                    "exact": matched["exact"], "relocations": matched["relocations"],
                    "different_offsets": matched["different_offsets"][:20]
                }
            except ValueError as error:
                row["functions"][name] = {"error": str(error)}
        results.append(row)
        print(snap, label, {k: (v.get("matched"), v.get("size"))
                             for k, v in row["functions"].items() if "matched" in v}, flush=True)
(OUT / "results.json").write_text(json.dumps(results, indent=2) + "\n")

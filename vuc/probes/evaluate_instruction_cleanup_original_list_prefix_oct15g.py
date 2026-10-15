#!/usr/bin/env python3
"""Original-order InstructionList predecessors for arena cleanup __tcf_1.

Tests real recovered methods from the same original Instruction.cpp region
rather than fabricated register-selection shims. The 46-byte teardown must
retain three authentic relocations to qualify for a strict improvement.
"""
from pathlib import Path
import importlib.util
import itertools
import re
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/instruction-list-prefix-oct15g")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
config = VUC / "config/instruction_arena_cleanup_unit_oct14_probe.json"

text = (VUC / "src/InstructionListLineCommentUnit.cpp").read_text()
instr = (VUC / "probes/instruction_full_source_unit_oct13.cpp").read_text()
first = text.index("void InstructionList::Add(Instruction *")
header = text[:first].replace('#include "MLGameString.h"',
                              '#include "' + str(VUC / "src/MLGameString.h") + '"')
header = '#include <iostream>\n' + header
header = header.replace("MLGameString text;", "MLGameString text_;")
header = header.replace(" : text(name),", " : text_(name),")
header = header.replace("    static void *operator new(std::size_t);",
                        "    static char *instrData;\n    static void *operator new(std::size_t);")
names = re.compile(r"(?m)^(?:void|MLGameString) InstructionList::")
matches = list(names.finditer(text))
methods = {}
for i, found in enumerate(matches):
    start = found.start()
    end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
    body = text[start:end].strip()
    if "Add(Instruction *" in body[:95]: name = "add"
    elif "Add(const char *" in body[:95]: name = "addfmt"
    elif "AddB(" in body[:95]: name = "addb"
    elif "AddL(" in body[:95]: name = "addl"
    elif "AddLineComment(" in body[:95]: name = "line"
    elif "GenLabel(" in body[:95]: name = "label"
    elif "GenTmp(" in body[:95]: name = "tmp"
    elif "Print(" in body[:95]: name = "print"
    else: raise ValueError(body[:95])
    methods[name] = body
assert set(methods) == {"add", "addfmt", "addb", "addl", "line", "label", "tmp", "print"}
# The original 5-byte GenerateImage method has an empty genuine body.
header = header.replace("    void AddLineComment(int line);",
                        "    void AddLineComment(int line);\n    void GenerateImage(unsigned char *);")
methods["image"] = "void InstructionList::GenerateImage(unsigned char *) {}"
orig = instr[instr.index("const char *Instruction::ToString()"):]
suffix = "char *Instruction::instrData;\n" + orig

order = ["add", "addfmt", "addb", "addl", "line", "image", "label", "tmp", "print"]
seqs = [
    [],
    ["print"],
    ["tmp", "print"],
    ["label", "tmp", "print"],
    ["image", "label", "tmp", "print"],
    ["line", "image", "label", "tmp", "print"],
    ["addl", "line", "image", "label", "tmp", "print"],
    ["addb", "addl", "line", "image", "label", "tmp", "print"],
    ["addfmt", "addb", "addl", "line", "image", "label", "tmp", "print"],
    order,
    ["add", "addfmt", "addb", "addl", "line"],
    ["add", "addfmt", "addb", "addl", "line", "image"],
    ["add", "addfmt", "addb", "addl", "line", "image", "label"],
    ["add", "addfmt", "addb", "addl", "line", "image", "label", "tmp"],
]
flagsets = {
    "base": ["-O2", "-frename-registers", "-fuse-cxa-atexit", "-ffunction-sections"],
    "no-sections": ["-O2", "-frename-registers", "-fuse-cxa-atexit"],
    "plain": ["-O2", "-fuse-cxa-atexit", "-ffunction-sections"],
}
records = []
best = None
for i, (seq, (flagname, flags)) in enumerate(itertools.product(seqs, flagsets.items())):
    name = f"{i:03d}-{'-'.join(seq) if seq else 'none'}-{flagname}"
    content = header + "\n" + "\n".join(methods[x] for x in seq) + "\n" + suffix
    cpp = OUT / (name + ".cpp")
    obj = OUT / (name + ".o")
    cpp.write_text(content)
    cp = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215",
                         *flags, "-c", str(cpp), "-o", str(obj)],
                        capture_output=True, text=True)
    if cp.returncode:
        records.append({"name": name, "error": cp.stderr[-160:]})
        continue
    try:
        result = matcher.run(obj, config)
    except ValueError as e:
        records.append({"name": name, "error": str(e)})
        continue
    row = {"name": name, "length": result["length"],
           "matched": result["exact_bytes"], "relocations": result["relocations"],
           "different": result["different_offsets"]}
    records.append(row)
    if row["length"] != 46 or row["relocations"] != 3:
        continue
    if best is None or row["matched"] > best["matched"]:
        best = row
        (OUT / "best.cpp").write_text(content)
        (OUT / "best.o").write_bytes(obj.read_bytes())
        (OUT / "best.flags").write_text(" ".join(flags) + "\n")
        print("BEST", json.dumps(best), flush=True)
    if row["matched"] == 46:
        break
(OUT / "results.json").write_text(json.dumps(records, indent=2) + "\n")
print(json.dumps({"tested": len(records), "best": best,
                  "valid": sum(r.get("length") == 46 and r.get("relocations") == 3 for r in records)}, indent=2))

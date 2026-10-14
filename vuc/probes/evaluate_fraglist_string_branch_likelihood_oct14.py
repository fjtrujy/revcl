#!/usr/bin/env python3
"""GCC 3.3.3 branch-frequency controls for a real MLGameString list copy.

Start from the shortest original-call-graph source with memcpy; vary only
the three observable decisions and historical assembler alignment.
"""
import importlib.util
import itertools
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("match", root / "vuc/scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
elf, target = m.decomp.load()
address = 0x0806e5b0
sym = "_Z17TranslateFragListI12MLGameStringE11ASTNodeListIPN2IR13Fragment_DataEERS1_IT_E"
section = next(s for s in target["sections"] if s["name"] == ".text")
begin = section["offset"] + address - section["address"]
original = elf[begin:begin + 306]
targets = {
    "_Znwj": "0x080499ec",
    "_ZTVN2IR13Fragment_DataE": "0x08076068",
    "_ZN12MLGameStringC1ERKS_": "0x08071770",
    "_Znaj": "0x080499ac",
    "_ZdlPv": "0x0804973c",
    "_Unwind_Resume": "0x0804998c",
}
source = (root / "vuc/probes/translate_frag_list_string_candidate_oct14.cpp").read_text()
source = source.replace(
    '"../src/MLGameString.h"', '"' + str(root / "vuc/src/MLGameString.h") + '"'
).replace(
    "IR::Fragment_Data *fragment = new IR::Fragment_Data(input.entries[index]);",
    "const MLGameString &name = input.entries[index];\n"
    "        IR::Fragment_Data *fragment = new IR::Fragment_Data(name);"
)
work = Path("/tmp/vuc-binutils215/fraglist-string-branch-hints-oct14")
work.mkdir(parents=True, exist_ok=True)
rows = []
best = None
variants = {"plain": lambda x: x,
            "likely": lambda x: f"__builtin_expect(({x}), 1)",
            "unlikely": lambda x: f"__builtin_expect(({x}), 0)"}
for (loop_name, loop_hint), (grow_name, grow_hint), (copy_name, copy_hint), align, reorder in itertools.product(
    variants.items(), variants.items(), variants.items(), (1, 2, 4, 8), (True, False)
):
    code = source.replace("while (i < input.length)",
                          "while (" + loop_hint("i < input.length") + ")")
    code = code.replace("if (old_size >= capacity)",
                        "if (" + grow_hint("old_size >= capacity") + ")")
    code = code.replace("if (old_entries)",
                        "if (" + copy_hint("old_entries != 0") + ")")
    name = f"{loop_name}-{grow_name}-{copy_name}-j{align}-r{int(reorder)}"
    cpp = work / (name + ".cpp")
    obj = work / (name + ".o")
    cpp.write_text(code)
    flags = ["-O2", "-frename-registers", f"-falign-jumps={align}"]
    if not reorder:
        flags.append("-fno-reorder-blocks")
    result = subprocess.run(
        ["/tmp/vuc-binutils215/g++-jan25-as215", *flags,
         "-c", str(cpp), "-o", str(obj)], capture_output=True, text=True
    )
    if result.returncode:
        rows.append({"name": name, "error": result.stderr[:160]})
        continue
    try:
        candidate, relocations = m.prepare_named_function(
            obj.read_bytes(), sym, targets, address
        )
    except ValueError as exc:
        rows.append({"name": name, "error": str(exc)})
        continue
    matched = sum(x == y for x, y in zip(candidate, original))
    prefix = next((i for i, (x, y) in enumerate(zip(candidate, original))
                   if x != y), min(len(candidate), len(original)))
    row = {"name": name, "size": len(candidate), "relocations": relocations,
           "overlap": matched, "prefix": prefix}
    rows.append(row)
    if len(candidate) == 306 and relocations == 6:
        if best is None or (matched, prefix) > (best["overlap"], best["prefix"]):
            best = row
            (work / "best_original_length.cpp").write_text(code)
            (work / "best_original_length.o").write_bytes(obj.read_bytes())
(work / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
print(json.dumps({"attempted": len(rows), "original_length": 306,
                  "best_original_length": best,
                  "best_overlapping": sorted(
                      (r for r in rows if "overlap" in r),
                      key=lambda r: r["overlap"], reverse=True)[:8],
                  "results": str(work / "results.json")}, indent=2))

#!/usr/bin/env python3
"""Probe genuine TranslateFrag predecessor ownership for the 306-byte list.

All source is C++03, and the extra predecessor is the already reconstructed
original function rather than a fabricated peephole-state shifter.
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
original_binary, target = m.decomp.load()
address = 0x0806e5b0
symbol = "_Z17TranslateFragListI12MLGameStringE11ASTNodeListIPN2IR13Fragment_DataEERS1_IT_E"
text = next(s for s in target["sections"] if s["name"] == ".text")
offset = text["offset"] + address - text["address"]
original = original_binary[offset:offset + 306]
targets = {
    "_Znwj": "0x080499ec",
    "_ZTVN2IR13Fragment_DataE": "0x08076068",
    "_ZN12MLGameStringC1ERKS_": "0x08071770",
    "_Znaj": "0x080499ac",
    "_ZdlPv": "0x0804973c",
    "_Unwind_Resume": "0x0804998c",
    "_Z13TranslateFragRK12MLGameString": "0x0806dd30",
}
template = (root / "vuc/probes/translate_frag_list_string_candidate_oct14.cpp").read_text()
head = template.index("template <typename T> struct ASTNodeList")
with_real_predecessor = (
    '#include "' + str(root / "vuc/src/TranslateFrag.cpp") + '"\n\n'
    + template[head:]
)
copied_direct = ("IR::Fragment_Data *fragment = new IR::Fragment_Data(input.entries[index]);")
copy_statement = "__builtin_memcpy(entries, old_entries, length * sizeof(T));"
work = Path("/tmp/vuc-binutils215/fraglist-string-source-owner-oct14")
work.mkdir(parents=True, exist_ok=True)
records = []
best = None
for (call_name, call_replacement), copy_name, init_first, jump, reorder in itertools.product(
    (("direct", copied_direct),
     ("call", "IR::Fragment_Data *fragment = TranslateFrag(input.entries[index]);")),
    ("memcpy", "loop"), (False, True), (1, 2, 4, 8), (False, True)
):
    candidate = with_real_predecessor.replace(copied_direct, call_replacement)
    if copy_name == "loop":
        candidate = candidate.replace(
            copy_statement,
            "for (int j = 0; j < length; ++j) entries[j] = old_entries[j];"
        )
    if init_first:
        candidate = candidate.replace(
            "    ASTNodeList<IR::Fragment_Data *> output;\n    int i = 0;",
            "    int i = 0;\n    ASTNodeList<IR::Fragment_Data *> output;"
        )
    key = f"{call_name}-{copy_name}-init{int(init_first)}-j{jump}-r{int(reorder)}"
    filename = work / (key + ".cpp")
    obj = work / (key + ".o")
    filename.write_text(candidate)
    flags = ["-O2", "-frename-registers", f"-falign-jumps={jump}"]
    if not reorder:
        flags.append("-fno-reorder-blocks")
    result = subprocess.run(
        ["/tmp/vuc-binutils215/g++-jan25-as215", *flags, "-c", str(filename),
         "-o", str(obj)], capture_output=True, text=True
    )
    if result.returncode:
        records.append({"name": key, "error": result.stderr[:160]})
        continue
    try:
        code, reloc = m.prepare_named_function(obj.read_bytes(), symbol, targets, address)
    except ValueError as exc:
        records.append({"name": key, "error": str(exc)})
        continue
    score = sum(x == y for x, y in zip(code, original))
    prefix = next((i for i, (x, y) in enumerate(zip(code, original)) if x != y),
                  min(len(code), len(original)))
    row = {"name": key, "size": len(code), "relocations": reloc,
           "matching_original_offsets": score, "prefix": prefix}
    records.append(row)
    if len(code) == 306 and reloc == 6:
        if best is None or (score, prefix) > (
            best["matching_original_offsets"], best["prefix"]
        ):
            best = row
            (work / "best_original_length.cpp").write_text(candidate)
            (work / "best_original_length.o").write_bytes(obj.read_bytes())

(work / "results.json").write_text(json.dumps(records, indent=2) + "\n")
print(json.dumps({"original": "0x0806e5b0", "size": 306,
                  "attempted": len(records), "best_original_length": best,
                  "best_overlapping": sorted(
                      (r for r in records if "matching_original_offsets" in r),
                      key=lambda x: x["matching_original_offsets"], reverse=True)[:8],
                  "results": str(work / "results.json")}, indent=2))

#!/usr/bin/env python3
"""Test ordinary GCC 3.3 source lifetimes against the original 306-byte list.

All compilations are scratch-only. An original-length, relocation-authenticated
partial is diagnostic; only a full exact match could enter vuc/src.
"""
import importlib.util
import itertools
import json
import subprocess
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_match", ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)

original_elf, target = match.decomp.load()
function = "_Z17TranslateFragListI12MLGameStringE11ASTNodeListIPN2IR13Fragment_DataEERS1_IT_E"
address = 0x0806e5b0
size = 306
sec = next(s for s in target["sections"] if s["name"] == ".text")
start = sec["offset"] + address - sec["address"]
original = original_elf[start:start + size]
relocations = {
    "_Znwj": "0x080499ec",
    "_ZTVN2IR13Fragment_DataE": "0x08076068",
    "_ZN12MLGameStringC1ERKS_": "0x08071770",
    "_Znaj": "0x080499ac",
    "_ZdlPv": "0x0804973c",
    "_Unwind_Resume": "0x0804998c",
}
source = (ROOT / "vuc/probes/translate_frag_list_string_candidate_oct14.cpp").read_text()
tmp = Path("/tmp/vuc-binutils215/fraglist-string-lifetime-oct14")
tmp.mkdir(exist_ok=True, parents=True)
copy_statement = "__builtin_memcpy(entries, old_entries, length * sizeof(T));"
copy_forms = {
    "memcpy": copy_statement,
    "memmove": "__builtin_memmove(entries, old_entries, length * sizeof(T));",
    "loop": "for (int j = 0; j < length; ++j) entries[j] = old_entries[j];",
}
index_forms = {
    "explicit": "int index = i;\n        ++i;",
    "postinc": "int index = i++;",
}
entry_forms = {
    "local": "IR::Fragment_Data *fragment = new IR::Fragment_Data(input.entries[index]);\n        output.Add(fragment);",
    "direct": "output.Add(new IR::Fragment_Data(input.entries[index]));",
    "reference": "const MLGameString &name = input.entries[index];\n        IR::Fragment_Data *fragment = new IR::Fragment_Data(name);\n        output.Add(fragment);",
}
saved_entry = ("IR::Fragment_Data *fragment = new IR::Fragment_Data(input.entries[index]);\n"
               "        output.Add(fragment);")
flag_sets = []
for align, reorder in itertools.product((1, 2, 4, 8), (True, False)):
    flags = ["-O2", "-frename-registers", f"-falign-jumps={align}"]
    if not reorder:
        flags.append("-fno-reorder-blocks")
    flag_sets.append((f"j{align}-r{int(reorder)}", flags))
for align, extra in itertools.product((1, 4), (
    "-falign-labels=1", "-falign-loops=1",
    "-fno-guess-branch-probability",
)):
    flag_sets.append((f"j{align}-{extra[2:]}", [
        "-O2", "-frename-registers", f"-falign-jumps={align}", extra
    ]))
record = []
lengths = Counter()
best = None
compiler = "/tmp/vuc-binutils215/g++-jan25-as215"
for (copy_name, copy), (index_name, index), (entry_name, entry), init_first, (flag_name, flags) in itertools.product(
    copy_forms.items(), index_forms.items(), entry_forms.items(), (False, True), flag_sets
):
    variant = source.replace(copy_statement, copy).replace(
        '"../src/MLGameString.h"', '"' + str(ROOT / "vuc/src/MLGameString.h") + '"'
    )
    variant = variant.replace("int index = i;\n        ++i;", index)
    variant = variant.replace(saved_entry, entry)
    if init_first:
        variant = variant.replace("    ASTNodeList<IR::Fragment_Data *> output;\n    int i = 0;",
                                  "    int i = 0;\n    ASTNodeList<IR::Fragment_Data *> output;")
    suffix = f"{copy_name}-{index_name}-{entry_name}-init{int(init_first)}-{flag_name}"
    path = tmp / (suffix + ".cpp")
    obj = tmp / (suffix + ".o")
    path.write_text(variant)
    cp = subprocess.run([compiler, *flags, "-c", str(path), "-o", str(obj)],
                        capture_output=True, text=True)
    if cp.returncode:
        record.append({"variant": suffix, "error": cp.stderr[:180]})
        continue
    try:
        normalized, nrel = match.prepare_named_function(
            obj.read_bytes(), function, relocations, address
        )
    except Exception as exc:
        record.append({"variant": suffix, "error": str(exc)})
        continue
    length = len(normalized)
    lengths[length] += 1
    overlap = sum(x == y for x, y in zip(normalized, original))
    prefix = next((i for i, (x, y) in enumerate(zip(normalized, original))
                   if x != y), min(length, size))
    result = {"variant": suffix, "size": length, "relocations": nrel,
              "overlap": overlap, "prefix": prefix}
    record.append(result)
    if length == size and nrel == 6:
        if best is None or (overlap, prefix) > (best["overlap"], best["prefix"]):
            best = result
            (tmp / "best_original_length.cpp").write_text(variant)
            (tmp / "best_original_length.o").write_bytes(obj.read_bytes())

(tmp / "results.json").write_text(json.dumps(record, indent=2) + "\n")
print(json.dumps({"function": function, "original_address": hex(address),
                  "original_size": size, "attempted": len(record),
                  "generated_sizes": dict(lengths), "best_original_length": best,
                  "best_overlapping": sorted((r for r in record if "overlap" in r),
                                           key=lambda r: (r["overlap"], r["prefix"]),
                                           reverse=True)[:8],
                  "results": str(tmp / "results.json")}, indent=2))

#!/usr/bin/env python3
"""Recover VUC translation-unit filename evidence from ELF STT_FILE records.

Local symbols between STT_FILE entries are assigned by the assembler to that
file. Global symbols follow the local symbol region and cannot safely be
attributed to any one translation unit from this ordering alone.
"""
import argparse
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_decomp", ROOT / "vuc/scripts/decomp.py")
decomp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decomp)
OUTPUT = ROOT / "vuc/config/source_units.tsv"


def recover(info, raw):
    sections = info["sections"]
    symtab = next(s for s in sections if s["name"] == ".symtab")
    strtab = sections[symtab["link"]]
    names = raw[strtab["offset"]:strtab["offset"] + strtab["size"]]
    if symtab["entry_size"] != decomp.SYM.size:
        raise ValueError("unsupported ELF symbol entry size")

    files = []
    current = None
    total_records = 0
    for index, at in enumerate(range(symtab["offset"],
                                     symtab["offset"] + symtab["size"], decomp.SYM.size)):
        name_off, address, size, flags, _, section_index = decomp.SYM.unpack_from(raw, at)
        kind, binding = flags & 15, flags >> 4
        if kind == 4:  # STT_FILE
            total_records += 1
            name = decomp.cstr(names, name_off)
            current = None
            if name.endswith(".cpp"):
                current = {"file": name, "index": index, "local_functions": []}
                files.append(current)
        elif current is not None and binding == 0 and kind == 2 and size:
            if section_index < len(sections) and sections[section_index]["name"] == ".text":
                current["local_functions"].append({
                    "name": decomp.cstr(names, name_off), "address": address, "size": size})
        elif binding != 0:
            # ELF symbol tables place non-local bindings after the local region.
            current = None
    return files, total_records


def tsv(files):
    lines = ["file\tsymbol_index\tlocal_text_function_count\tlocal_text_addresses"]
    for unit in files:
        addresses = (";".join("0x%08x" % fn["address"] for fn in unit["local_functions"])
                     or "-")
        lines.append("%s\t%d\t%d\t%s" %
                     (unit["file"], unit["index"], len(unit["local_functions"]), addresses))
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true",
                        help="write deterministic VUC translation-unit inventory")
    args = parser.parse_args()
    raw, info = decomp.load()
    units, count = recover(info, raw)
    print("%d STT_FILE entries; %d original .cpp translation-unit records" %
          (count, len(units)))
    print("Local function records: %d" %
          sum(len(unit["local_functions"]) for unit in units))
    if args.write:
        OUTPUT.write_text(tsv(units))
        print("wrote " + str(OUTPUT.relative_to(ROOT)))
    else:
        print(tsv(units), end="")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Inventory unmatched named, sized VUC data objects from the canonical ELF.

Accepted objects come ONLY from the explicit exact_data_candidates.json gate.
Uninitialized BSS and unnamed literals are not byte-covered by this report.
A probe that happens to compare exactly is not accepted until its source and
manifest pass the separate strict data acceptance rules.
"""
import argparse
from collections import Counter
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CFG = ROOT/"vuc/config"
spec = importlib.util.spec_from_file_location("vuc_match_data",
                                              ROOT/"vuc/scripts/match_data.py")
data = importlib.util.module_from_spec(spec)
spec.loader.exec_module(data)
SECTIONS = frozenset((".data", ".rodata", ".bss"))


def inventory():
    raw, target = data.match.decomp.load()
    named_sections, headers = data.match.elf_sections(raw)
    labels = {index: name for name, (index, _) in named_sections.items()}
    original = {}
    for name, entry in data.read_named_symbols(raw, named_sections, headers):
        addr, size, typeinfo, index = entry[1], entry[2], entry[3] & 15, entry[5]
        if not name or typeinfo != 1 or size <= 0 or labels.get(index) not in SECTIONS:
            continue
        key = (name, addr)
        if key in original:
            raise ValueError("duplicate original sized named object: "+name)
        original[key] = {
            "symbol": name, "address": "0x%08x" % addr,
            "size": size, "section": labels[index],
        }
    accepted = set()
    for relative in json.loads((CFG/"exact_data_candidates.json").read_text()):
        path = (ROOT/relative).resolve()
        if not path.is_relative_to(CFG.resolve()):
            raise ValueError("accepted manifest escapes data configuration directory")
        manifest = json.loads(path.read_text())
        key = manifest["symbol"], int(manifest["address"], 0)
        if key not in original:
            raise ValueError("accepted data manifest has no original named object: "+str(key))
        if key in accepted:
            raise ValueError("duplicate accepted data manifest: "+str(key))
        if (original[key]["size"] != manifest["size"]
                or original[key]["section"] != manifest["section"]):
            raise ValueError("accepted data symbol metadata differs from original")
        accepted.add(key)
    rows = [dict(record, status="EXACT" if key in accepted else "UNRESOLVED")
            for key, record in sorted(original.items(),
                                      key=lambda pair: (pair[1]["section"],
                                                        int(pair[1]["address"], 0),
                                                        pair[1]["symbol"]))]
    outstanding = [record for record in rows if record["status"] != "EXACT"]
    return {
        "scope": "original_named_sized_ELF_STT_OBJECT_in_data_rodata_bss",
        "whole_binary_score": None,
        "named_data_symbols": len(rows),
        "exact_data_symbols": len(accepted),
        "unresolved_data_symbols": len(outstanding),
        "unresolved_by_section": dict(sorted(Counter(r["section"] for r in outstanding).items())),
        "unresolved": outstanding,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="optional output JSON path")
    parser.add_argument("--summary-only", action="store_true")
    args = parser.parse_args()
    result = inventory()
    if args.summary_only:
        result = {key: value for key, value in result.items() if key != "unresolved"}
    encoded = json.dumps(result, indent=2)+"\n"
    if args.output:
        args.output.write_text(encoded)
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()

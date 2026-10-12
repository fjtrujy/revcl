#!/usr/bin/env python3
"""Strict original-ELF byte comparison for one named VUC data object.

Unlike a function-level code match, this resolves the candidate's own
ELF32 object relocations against independently authenticated original
named function/object targets, then compares every original data byte.

The candidate must be a *compiled* ELF32 relocatable object; the source may
not contain original linked addresses or fabricated machine/data blobs.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("match_probe", ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)


def read_named_symbols(raw, sections, headers):
    """Return name, raw ELF symbol, and address/section information."""
    symsection = sections[".symtab"][1]
    strings = match.section_bytes(raw, headers[symsection[6]])
    result = []
    for position in range(symsection[4], symsection[4] + symsection[5], match.decomp.SYM.size):
        entry = match.decomp.SYM.unpack_from(raw, position)
        name = match.decomp.cstr(strings, entry[0])
        result.append((name, entry))
    return result


def original_symbol(raw, target, label, address=None, size=None, kind=None):
    """Reject absent, duplicated, incorrectly sized or relocated original symbols."""
    sections = target["sections"]
    section_names = {i: entry["name"] for i, entry in enumerate(sections)}
    names = {entry["name"]: entry for entry in sections}
    symsec = names[".symtab"]
    stringsec = sections[symsec["link"]]
    strings = raw[stringsec["offset"]:stringsec["offset"] + stringsec["size"]]
    found = []
    for off in range(symsec["offset"], symsec["offset"] + symsec["size"], match.decomp.SYM.size):
        name, value, length, info, _, section_index = match.decomp.SYM.unpack_from(raw, off)
        if match.decomp.cstr(strings, name) != label:
            continue
        if address is not None and value != int(address, 0):
            continue
        if size is not None and length != size:
            continue
        if kind is not None and info & 15 != kind:
            continue
        found.append((value, length, section_index, section_names.get(section_index)))
    if len(found) != 1:
        raise ValueError("expected one authenticated original ELF symbol: " + label)
    return found[0]


def compare_named_data(candidate_raw, config, original_raw, original_target):
    label = config["symbol"]
    addr = int(config["address"], 0)
    size = int(config["size"])
    if size <= 0:
        raise ValueError("invalid named data object size")
    original = original_symbol(original_raw, original_target, label,
                               config["address"], size, 1)
    if original[3] != config["section"]:
        raise ValueError("original data symbol is not in the configured section")
    original_section = original_target["sections"][original[2]]
    if not original_section["address"] <= addr <= original_section["address"] + original_section["size"] - size:
        raise ValueError("original data object outside its section")
    original_offset = original_section["offset"] + addr - original_section["address"]
    target_data = original_raw[original_offset:original_offset + size]

    candidate_sections, headers = match.elf_sections(candidate_raw)
    symbols = read_named_symbols(candidate_raw, candidate_sections, headers)
    definitions = [(name, sym) for name, sym in symbols
                   if name == label and sym[3] & 15 == 1 and sym[4] == 0
                   and 0 < sym[5] < len(headers)]
    if len(definitions) != 1:
        raise ValueError("expected exactly one defined candidate data symbol: " + label)
    _, entry = definitions[0]
    if entry[2] != size:
        raise ValueError("candidate data object has wrong size")
    section_index = entry[5]
    sec = headers[section_index]
    start = entry[1]
    if sec[1] == 8 or start + size > sec[5]:
        raise ValueError("candidate data object missing from file-backed section")
    source_data = bytearray(match.section_bytes(candidate_raw, sec)[start:start + size])
    expected = {int(offset, 0): record for offset, record in config["relocations"].items()}
    seen = set()
    for rel in headers:
        if rel[1] != 9 or rel[7] != section_index:  # SHT_REL, sh_info = target
            continue
        if rel[9] != 8:
            raise ValueError("unexpected ELF32 relocation record size")
        for pos in range(rel[4], rel[4] + rel[5], 8):
            offset, info = struct.unpack_from("<II", candidate_raw, pos)
            if offset < start or offset >= start + size:
                continue
            local = offset - start
            if local in seen or local not in expected or local + 4 > size:
                raise ValueError("duplicate, unknown or truncated data relocation")
            kind = info & 255
            symbol_index = info >> 8
            if kind != 1 or symbol_index >= len(symbols):
                raise ValueError("unsupported data relocation kind or symbol index")
            candidate_name, symbol = symbols[symbol_index]
            record = expected[local]
            if candidate_name != record["symbol"]:
                raise ValueError("data relocation symbol does not match original manifest")
            original_symbol(original_raw, original_target, candidate_name,
                            record["address"], kind=record["type"])
            raw_addend = struct.unpack_from("<i", source_data, local)[0]
            real_address = int(record["address"], 0)
            # For STT_FUNC/STT_OBJECT, the candidate symbol's st_value
            # is *not* a second copy of the original linked offset.
            real_address += match.linked_symbol_offset(symbol, len(headers))
            effective = match.relocate_word(kind, real_address, raw_addend,
                                             addr + local)
            struct.pack_into("<I", source_data, local, effective & 0xffffffff)
            seen.add(local)
    if seen != set(expected):
        raise ValueError("original data relocation omitted from compiled candidate")
    different = [i for i, (a, b) in enumerate(zip(source_data, target_data)) if a != b]
    return {
        "symbol": label,
        "original_address": config["address"],
        "size": size,
        "exact_bytes": size - len(different),
        "exact": not different,
        "different_offsets": different,
        "resolved_relocations": len(seen),
    }


def run(candidate_path, config_path):
    config = json.loads(Path(config_path).read_text())
    original_raw, original_target = match.decomp.load()
    return compare_named_data(Path(candidate_path).read_bytes(), config,
                              original_raw, original_target)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--object", required=True)
    parser.add_argument("--config", required=True)
    args = parser.parse_args()
    report = run(args.object, args.config)
    print(json.dumps(report, indent=2))
    return 0 if report["exact"] else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, struct.error) as exc:
        print("match-data:", exc, file=sys.stderr)
        sys.exit(2)

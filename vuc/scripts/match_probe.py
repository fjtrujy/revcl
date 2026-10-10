#!/usr/bin/env python3
"""Strict relocation-resolved code comparison for a single VUC probe function.

This is an experimental exact-byte comparator, not the project-wide scorer.
Only i386 R_386_32 and R_386_PC32 are supported; unknown relocations fail.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_decomp", ROOT / "vuc/scripts/decomp.py")
decomp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decomp)
CONFIG = ROOT / "vuc/config/asm_output_probe.json"


def elf_sections(raw):
    h = decomp.ELF_HEADER.unpack_from(raw)
    if raw[:6] != b"\x7fELF\x01\x01" or h[2] != 3:
        raise ValueError("expected ELF32 i386")
    off, count, entsize, names_index = h[6], h[12], h[11], h[13]
    if entsize != decomp.SHDR.size:
        raise ValueError("unsupported ELF section header format")
    sh = [decomp.SHDR.unpack_from(raw, off + i * entsize) for i in range(count)]
    stringtab = raw[sh[names_index][4]:sh[names_index][4] + sh[names_index][5]]
    return {decomp.cstr(stringtab, item[0]): (index, item) for index, item in enumerate(sh)}, sh


def section_bytes(raw, sec):
    if sec[1] == 8:  # SHT_NOBITS
        return b""
    return raw[sec[4]:sec[4] + sec[5]]


def prepare_object(raw, reloc_targets, original_address):
    sections, headers = elf_sections(raw)
    if ".rel.text" not in sections or ".symtab" not in sections:
        raise ValueError("candidate must contain .rel.text and .symtab")
    text = bytearray(section_bytes(raw, sections[".text"][1]))
    symsec = sections[".symtab"][1]
    symnames = section_bytes(raw, headers[symsec[6]])
    symbols = []
    for pos in range(symsec[4], symsec[4] + symsec[5], decomp.SYM.size):
        item = decomp.SYM.unpack_from(raw, pos)
        symname = decomp.cstr(symnames, item[0])
        if item[5] < len(headers) and item[3] & 15 == 3:  # STT_SECTION
            for name, (index, _) in sections.items():
                if index == item[5]:
                    symname = name
                    break
        symbols.append((symname, item[1]))

    relsec = sections[".rel.text"][1]
    count = 0
    for pos in range(relsec[4], relsec[4] + relsec[5], 8):
        offset, info = struct.unpack_from("<II", raw, pos)
        symidx, kind = info >> 8, info & 0xff
        if symidx >= len(symbols) or offset + 4 > len(text):
            raise ValueError("bad text relocation")
        symbol, value = symbols[symidx]
        if symbol not in reloc_targets:
            raise ValueError("unmapped relocation target: " + symbol)
        address = int(reloc_targets[symbol], 0) + value
        addend = struct.unpack_from("<i", text, offset)[0]
        new_value = relocate_word(kind, address, addend, original_address + offset)
        struct.pack_into("<I", text, offset, new_value & 0xffffffff)
        count += 1
    return bytes(text), count


def relocate_word(kind, address, addend, place):
    if kind == 1:  # R_386_32
        return (address + addend) & 0xffffffff
    if kind == 2:  # R_386_PC32
        return (address + addend - place) & 0xffffffff
    raise ValueError("unsupported relocation type %d" % kind)


def compare_string_sections(raw, mapping, target_data, target):
    """Verify the entire emitted literal section at its mapped original address."""
    obj_sections, _ = elf_sections(raw)
    target_rodata = next(s for s in target["sections"] if s["name"] == ".rodata")
    count = 0
    for name, address in mapping.items():
        if not name.startswith(".rodata") or name not in obj_sections:
            continue
        candidate = section_bytes(raw, obj_sections[name][1])
        start = target_rodata["offset"] + int(address, 0) - target_rodata["address"]
        if start < target_rodata["offset"] or start + len(candidate) > target_rodata["offset"] + target_rodata["size"]:
            raise ValueError("mapped literal section outside original .rodata")
        if candidate != target_data[start:start + len(candidate)]:
            raise ValueError("literal section bytes differ at " + address)
        count += len(candidate)
    return count


def compare(candidate, original):
    if len(candidate) != len(original):
        raise ValueError("function lengths differ: %d vs %d" % (len(candidate), len(original)))
    positions = [i for i, (a, b) in enumerate(zip(candidate, original)) if a != b]
    return {"length": len(original), "exact_bytes": len(original) - len(positions),
            "exact": not positions, "different_offsets": positions}


def run(object_path, config_path=CONFIG):
    config = json.loads(Path(config_path).read_text())
    data, target = decomp.load()
    original_address = int(config["address"], 0)
    original_size = config["size"]
    src = next(x for x in target["sections"] if x["name"] == ".text")
    start = src["offset"] + original_address - src["address"]
    if original_address < src["address"] or start + original_size > src["offset"] + src["size"]:
        raise ValueError("original function outside .text")
    original = data[start:start + original_size]
    object_bytes = Path(object_path).read_bytes()
    literal_count = compare_string_sections(object_bytes, config["relocations"], data, target)
    candidate, reloc_count = prepare_object(object_bytes,
                                            config["relocations"], original_address)
    result = compare(candidate, original)
    result["relocations"] = reloc_count
    result["verified_literal_bytes"] = literal_count
    result["compiler_object"] = str(object_path)
    result["function"] = config["function"]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--object", required=True)
    args = parser.parse_args()
    result = run(args.object)
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, KeyError, struct.error) as exc:
        print("match-probe:", exc, file=sys.stderr)
        sys.exit(1)

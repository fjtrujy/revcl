#!/usr/bin/env python3
"""Inventory the canonical VUC ELF32 linker contract, never a match score.

All offsets and segment addresses come from the SHA-256-validated original.
The output is an exact *reference layout*, not an assembled candidate.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

import decomp

ROOT = Path(__file__).resolve().parents[2]
DEFAULT = ROOT / "vuc/config/original_link_layout.json"
PHDR = struct.Struct("<IIIIIIII")
SHDR = decomp.SHDR
PT_LOAD = 1
SHT_NOBITS = 8
SHT_DYNSYM = 11
SHT_REL = 9
SHF_ALLOC = 2
PT_TYPES = {
    1: "LOAD", 2: "DYNAMIC", 3: "INTERP", 4: "NOTE", 6: "PHDR",
    0x6474E550: "GNU_EH_FRAME", 0x6474E551: "GNU_STACK",
}


def subtract_intervals(start, end, covered):
    """Return non-section file ranges of a LOAD segment, including headers."""
    gaps = []
    cursor = start
    for lo, hi in sorted(covered):
        lo, hi = max(lo, start), min(hi, end)
        if hi <= lo:
            continue
        if lo > cursor:
            gaps.append({"file_offset": cursor, "size": lo - cursor})
        cursor = max(cursor, hi)
    if cursor < end:
        gaps.append({"file_offset": cursor, "size": end - cursor})
    return gaps


def candidate_section_inventory(raw):
    """Read candidate section names without consulting original section names."""
    if len(raw) < decomp.ELF_HEADER.size:
        raise ValueError("candidate too short for ELF32")
    h = decomp.ELF_HEADER.unpack_from(raw)
    shoff, size, count, names_index = h[6], h[11], h[12], h[13]
    if size != SHDR.size or names_index >= count:
        raise ValueError("invalid candidate section header dimensions")
    if shoff + size * count > len(raw):
        raise ValueError("candidate section table out of bounds")
    string_header = SHDR.unpack_from(raw, shoff + names_index * size)
    start, length = string_header[4], string_header[5]
    if start + length > len(raw):
        raise ValueError("candidate section names out of bounds")
    strings = raw[start:start + length]
    sections = []
    for index in range(count):
        name, kind, flags, address, offset, count_bytes, _, _, _, _ = SHDR.unpack_from(
            raw, shoff + index * size)
        if name >= len(strings):
            raise ValueError("invalid candidate section name")
        sections.append({
            "index": index, "name": decomp.cstr(strings, name), "type": kind,
            "flags": flags, "address": address, "offset": offset,
            "size": count_bytes,
        })
    return {"sections": sections}


def inspect_candidate(raw):
    result = parse_layout(raw, candidate_section_inventory(raw))
    result["scope"] = "candidate-ELF32-layout-only"
    result.pop("original_sha256")
    result["candidate_sha256"] = hashlib.sha256(raw).hexdigest()
    return result


def compare_link_layout(original, candidate):
    """Compare *only* linked metadata, never count verified code/data bytes."""
    fields = ("elf_header", "program_headers", "sections", "dynamic_relocations")
    differences = []
    if original["original_file_size"] != candidate["original_file_size"]:
        differences.append({
            "category": "file_size", "original": original["original_file_size"],
            "candidate": candidate["original_file_size"],
        })
    for category in fields:
        lhs, rhs = original[category], candidate[category]
        if isinstance(lhs, dict):
            lhs, rhs = [lhs], [rhs]
        if len(lhs) != len(rhs):
            differences.append({
                "category": category, "field": "record_count",
                "original": len(lhs), "candidate": len(rhs),
            })
        for index, (before, after) in enumerate(zip(lhs, rhs)):
            for name in sorted(set(before) | set(after)):
                if before.get(name) != after.get(name):
                    differences.append({
                        "category": category, "index": index, "field": name,
                        "original": before.get(name),
                        "candidate": after.get(name),
                    })
    return {
        "scope": "ELF32-linker-layout-structure-only",
        "layout_equal": not differences,
        "difference_count": len(differences),
        "differences": differences[:50],
        "differences_truncated": len(differences) > 50,
        "whole_elf_byte_exact": None,
        "verified_instruction_or_data_bytes": None,
    }


def parse_layout(raw, info):
    header = decomp.ELF_HEADER.unpack_from(raw)
    (ident, etype, machine, version, entry, phoff, shoff, flags,
     ehsize, phentsize, phnum, shentsize, shnum, shstrndx) = header
    if (ident[:6] != b"\x7fELF\x01\x01" or etype != 2 or
            machine != 3 or version != 1 or ehsize != decomp.ELF_HEADER.size or
            phentsize != PHDR.size or shentsize != SHDR.size):
        raise ValueError("unexpected ELF32 executable header")
    if phoff + phnum * phentsize > len(raw) or shoff + shnum * shentsize > len(raw):
        raise ValueError("ELF header tables extend beyond the file")
    if shnum != len(info["sections"]):
        raise ValueError("inconsistent original section table")

    segments = []
    loads = []
    for index in range(phnum):
        (kind, offset, vaddr, paddr, filesz, memsz, pflags, align) = PHDR.unpack_from(
            raw, phoff + index * phentsize)
        if offset + filesz > len(raw):
            raise ValueError("program header file bytes out of bounds")
        if kind == PT_LOAD:
            if filesz > memsz:
                raise ValueError("LOAD segment has more file than memory bytes")
            if align > 1 and (offset - vaddr) % align:
                raise ValueError("LOAD segment violates virtual/file alignment")
        item = {
            "index": index, "type": PT_TYPES.get(kind, "0x%x" % kind),
            "type_id": kind, "file_offset": offset, "virtual_address": vaddr,
            "physical_address": paddr, "file_size": filesz,
            "memory_size": memsz, "flags": pflags, "align": align,
        }
        segments.append(item)
        if kind == PT_LOAD:
            loads.append(item)
    if not loads or not any(p["virtual_address"] <= entry <
                            p["virtual_address"] + p["memory_size"]
                            and p["flags"] & 1 for p in loads):
        raise ValueError("ELF entrypoint is not within an executable LOAD")

    sections = []
    mapped = {p["index"]: [] for p in loads}
    for index, section in enumerate(info["sections"]):
        header_section = SHDR.unpack_from(raw, shoff + index * shentsize)
        (name_offset, kind, shflags, address, offset, size, link, section_info,
         align, entsize) = header_section
        if (section["type"], section["flags"], section["address"],
                section["offset"], section["size"]) != (
                kind, shflags, address, offset, size):
            raise ValueError("original section inventory and header disagree")
        if kind != SHT_NOBITS and offset + size > len(raw):
            raise ValueError("file-backed section extends outside original")
        if align > 1 and address and (address % align):
            raise ValueError("allocated section address violates alignment")

        mapping = None
        if shflags & SHF_ALLOC and size:
            for segment in loads:
                begin = segment["virtual_address"]
                if not begin <= address or address + size > begin + segment["memory_size"]:
                    continue
                if kind != SHT_NOBITS:
                    base = segment["file_offset"] + address - begin
                    if (base != offset or offset + size >
                            segment["file_offset"] + segment["file_size"]):
                        continue
                mapping = segment["index"]
                if kind != SHT_NOBITS:
                    mapped[mapping].append((offset, offset + size))
                break
            if mapping is None:
                raise ValueError("allocated section outside linked LOAD: " +
                                 section["name"])
        sections.append({
            "index": index, "name": section["name"],
            "type": kind, "flags": shflags, "virtual_address": address,
            "file_offset": offset, "size": size, "link": link,
            "info": section_info, "align": align, "entry_size": entsize,
            "load_segment_index": mapping,
            "file_backed": kind != SHT_NOBITS,
        })

    for segment in loads:
        segment["non_section_file_ranges"] = subtract_intervals(
            segment["file_offset"],
            segment["file_offset"] + segment["file_size"],
            mapped[segment["index"]])

    dynamic_relocations = []
    for section in sections:
        if section["type"] != SHT_REL:
            continue
        if section["entry_size"] != 8 or section["size"] % 8:
            raise ValueError("malformed original i386 SHT_REL entries")
        dyn_index = section["link"]
        if dyn_index >= len(sections) or sections[dyn_index]["type"] != SHT_DYNSYM:
            raise ValueError("relocation section does not link to DYNSYM")
        dyn = sections[dyn_index]
        strings_index = dyn["link"]
        if strings_index >= len(sections) or dyn["entry_size"] != decomp.SYM.size:
            raise ValueError("malformed original dynamic symbol table")
        string_sec = sections[strings_index]
        strings = raw[string_sec["file_offset"]:
                      string_sec["file_offset"] + string_sec["size"]]
        for at in range(section["file_offset"],
                        section["file_offset"] + section["size"], 8):
            target, info_word = struct.unpack_from("<II", raw, at)
            symbol_index, kind = info_word >> 8, info_word & 255
            if symbol_index * dyn["entry_size"] >= dyn["size"]:
                raise ValueError("original relocation refers beyond DYNSYM")
            offset = dyn["file_offset"] + symbol_index * dyn["entry_size"]
            symbol_name_offset = decomp.SYM.unpack_from(raw, offset)[0]
            if symbol_name_offset >= len(strings):
                raise ValueError("original relocation has invalid dynamic symbol")
            if not any(p["virtual_address"] <= target and target + 4 <=
                       p["virtual_address"] + p["memory_size"] for p in loads):
                raise ValueError("original relocation target outside LOAD segments")
            dynamic_relocations.append({
                "relocation_section": section["name"],
                "target_virtual_address": target, "type": kind,
                "dynamic_symbol_index": symbol_index,
                "dynamic_symbol_name": decomp.cstr(strings, symbol_name_offset),
            })

    return {
        "scope": "canonical-original-ELF32-layout-reference-only",
        "original_sha256": decomp.CANONICAL_SHA256,
        "original_file_size": len(raw),
        "elf_header": {
            "type": etype, "machine": machine, "entry": entry,
            "program_header_offset": phoff, "program_header_count": phnum,
            "section_header_offset": shoff, "section_header_count": shnum,
            "section_name_index": shstrndx,
            "header_size": ehsize, "program_header_entry_size": phentsize,
            "section_header_entry_size": shentsize, "flags": flags,
        },
        "program_headers": segments,
        "sections": sections,
        "dynamic_relocations": dynamic_relocations,
        "linked_reconstruction_verified": False,
        "whole_elf_byte_exact": False,
        "matching_executable_file_bytes": None,
    }


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--write", action="store_true", help="rewrite pinned reference JSON")
    group.add_argument("--check", action="store_true", help="compare against committed reference")
    group.add_argument("--summary", action="store_true", help="print only selected facts")
    group.add_argument("--compare-candidate", type=Path,
                       help="compare a separately rebuilt ELF layout, not its bytes")
    args = parser.parse_args(argv)
    raw, parsed = decomp.load()
    result = parse_layout(raw, parsed)
    if args.write:
        DEFAULT.write_text(json.dumps(result, indent=2) + "\n")
        print("Wrote authenticated original linker reference:", DEFAULT)
    elif args.check:
        if json.loads(DEFAULT.read_text()) != result:
            raise ValueError("original linker layout differs from committed reference")
        print("Original linker-layout reference: PASS")
    elif args.summary:
        print("Original ELF bytes:", result["original_file_size"])
        print("ELF entry: 0x%08x" % result["elf_header"]["entry"])
        for p in result["program_headers"]:
            if p["type"] == "LOAD":
                print("LOAD #%d: file=%#x..%#x, virt=%#x..%#x, flags=%#x, gaps=%d" %
                      (p["index"], p["file_offset"], p["file_offset"]+p["file_size"],
                       p["virtual_address"], p["virtual_address"]+p["memory_size"],
                       p["flags"], len(p["non_section_file_ranges"])))
        print("Original dynamic relocation entries:",
              len(result["dynamic_relocations"]))
        print("Linked candidate and whole-ELF equality: NOT ESTABLISHED")
    elif args.compare_candidate:
        candidate = inspect_candidate(args.compare_candidate.read_bytes())
        comparison = compare_link_layout(result, candidate)
        print(json.dumps(comparison, indent=2))
        return 0 if comparison["layout_equal"] else 1
    else:
        print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError) as error:
        print("original-link-layout: " + str(error), file=sys.stderr)
        sys.exit(1)

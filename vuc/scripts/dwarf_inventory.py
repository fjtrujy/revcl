#!/usr/bin/env python3
"""Inventory VUC's *actual* DWARF compilation units from its immutable ZIP.

This deliberately reads DWARF compile-unit root DIEs only. It does not infer
application debug types from the separately preserved ELF symbol table.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_decomp", ROOT / "vuc/scripts/decomp.py")
decomp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decomp)
OUTPUT = ROOT / "vuc/config/dwarf_units.json"


def leb(data, pos):
    result, shift = 0, 0
    for _ in range(10):
        if pos >= len(data):
            raise ValueError("truncated DWARF LEB128")
        b = data[pos]
        pos += 1
        result |= (b & 127) << shift
        if not b & 128:
            return result, pos
        shift += 7
    raise ValueError("oversized DWARF LEB128")


def abbreviations(raw, base):
    table = {}
    p = base
    while True:
        code, p = leb(raw, p)
        if code == 0:
            return table
        tag, p = leb(raw, p)
        if p >= len(raw):
            raise ValueError("truncated DWARF abbreviation")
        children = raw[p]
        p += 1
        attrs = []
        while True:
            name, p = leb(raw, p)
            form, p = leb(raw, p)
            if name == 0 and form == 0:
                break
            attrs.append((name, form))
        table[code] = (tag, children, attrs)


def attribute(raw, p, form, addr_size, strings):
    if form == 0x01:  # DW_FORM_addr
        width = addr_size
    elif form in (0x05, 0x12):
        width = 2
    elif form in (0x06, 0x0e, 0x13, 0x10):
        width = 4
    elif form in (0x07, 0x14):
        width = 8
    elif form in (0x0b, 0x0c, 0x11):
        width = 1
    else:
        width = None
    if width is not None:
        if p + width > len(raw):
            raise ValueError("truncated DWARF attribute")
        value = int.from_bytes(raw[p:p + width], "little")
        p += width
        if form == 0x0e:  # DW_FORM_strp
            value = decomp.cstr(strings, value)
        return value, p
    if form == 0x08:
        value = decomp.cstr(raw, p)
        return value, p + len(value.encode("utf-8")) + 1
    if form in (0x0f, 0x15):
        return leb(raw, p)
    if form == 0x16:
        real_form, p = leb(raw, p)
        return attribute(raw, p, real_form, addr_size, strings)
    if form in (0x03, 0x04, 0x09, 0x0a):
        length_width = {0x03: 2, 0x04: 4, 0x0a: 1}.get(form)
        if length_width is not None:
            length = int.from_bytes(raw[p:p + length_width], "little")
            p += length_width
        else:
            length, p = leb(raw, p)
        if p + length > len(raw):
            raise ValueError("truncated DWARF block")
        return raw[p:p + length].hex(), p + length
    if form == 0x0d:
        value, p = leb(raw, p)
        return (value >> 1) ^ -(value & 1), p
    raise ValueError("unsupported DWARF attribute form: 0x%x" % form)


def inventory(raw, target):
    sections = {s["name"]: s for s in target["sections"]}
    for name in (".debug_info", ".debug_abbrev", ".debug_str"):
        if name not in sections:
            return []
    def section(name):
        s = sections[name]
        return raw[s["offset"]:s["offset"] + s["size"]]
    info, abbrev, strings = (section(name) for name in
                             (".debug_info", ".debug_abbrev", ".debug_str"))
    units, p = [], 0
    while p < len(info):
        start = p
        if p + 11 > len(info):
            raise ValueError("truncated DWARF compilation-unit header")
        size = struct.unpack_from("<I", info, p)[0]
        if size >= 0xfffffff0:
            raise ValueError("unsupported DWARF64")
        end = p + 4 + size
        if end > len(info):
            raise ValueError("DWARF compilation unit extends beyond section")
        version, base, addr_size = struct.unpack_from("<HIB", info, p + 4)
        if version != 2 or addr_size != 4:
            raise ValueError("unexpected DWARF version or pointer size")
        p += 11
        code, p = leb(info, p)
        tags = abbreviations(abbrev, base)
        if code not in tags:
            raise ValueError("unknown root abbreviation")
        tag, _, attrs = tags[code]
        if tag != 0x11:  # DW_TAG_compile_unit
            raise ValueError("unexpected DWARF root tag")
        props = {}
        for key, form in attrs:
            value, p = attribute(info, p, form, addr_size, strings)
            if key in (0x03, 0x1b, 0x25, 0x11, 0x12):
                props[{0x03: "name", 0x1b: "comp_dir", 0x25: "producer",
                       0x11: "low_pc", 0x12: "high_pc"}[key]] = value
        props["offset"] = start
        units.append(props)
        p = end
    return units


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    raw, target = decomp.load()
    units = inventory(raw, target)
    result = {"scope": "DWARF compile-unit roots only",
              "compilation_units": len(units),
              "application_units": [u for u in units if not
                                    ("/glibc-" in u.get("comp_dir", "") or
                                     "/i386-libc/" in u.get("name", ""))],
              "units": units}
    if args.write:
        OUTPUT.write_text(json.dumps(result, indent=2) + "\n")
        print("wrote " + str(OUTPUT.relative_to(ROOT)))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()

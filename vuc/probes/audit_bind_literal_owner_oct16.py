#!/usr/bin/env python3
"""Audit original VUC .rodata ownership around Bind(ASTFunCall*) diagnostics.

Reads the canonical archive through the project ELF loader. It does not
modify or patch the binary. Direct immediate references establish users of
the nearby 'main', 'const', and recursion diagnostic strings, but do not
by themselves prove original C++ translation-unit membership.
"""
import importlib.util
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_decomp", ROOT / "vuc/scripts/decomp.py")
decomp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decomp)
raw, elf = decomp.load()
sections = {s["name"]: s for s in elf["sections"]}
rodata = sections[".rodata"]
code = sections[".text"]
code_data = raw[code["offset"]:code["offset"] + code["size"]]


def original_bytes(address, count):
    assert rodata["address"] <= address
    offset = rodata["offset"] + address - rodata["address"]
    return raw[offset:offset + count]


def absolute_references(address):
    pattern = struct.pack("<I", address)
    refs = []
    start = 0
    while True:
        position = code_data.find(pattern, start)
        if position < 0:
            return refs
        refs.append(code["address"] + position)
        start = position + 1


expected = (
    (0x08072301, b"main\0", (0x0804ea05,)),
    (0x08072331, b"const\0", (0x0804f309, 0x0804f3b5)),
    (0x08072340, b"' uses recursion! Recursion is forbidden in VUC!\0",
     (0x0804abb2,)),
)
for address, literal, refs in expected:
    assert original_bytes(address, len(literal)) == literal
    actual = absolute_references(address)
    assert all(reference in actual for reference in refs), (hex(address), actual)
    print("VERIFIED", hex(address), repr(literal), [hex(x) for x in refs])

padding = original_bytes(0x08072336, 10)
assert padding == b"\0" * 10, padding
print("VERIFIED original alignment bytes", hex(0x08072336), len(padding))
print("Bind candidate's 16-byte object padding is not owner-authenticated.")

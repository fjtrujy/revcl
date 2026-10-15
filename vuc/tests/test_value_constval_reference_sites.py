"""Original VUC Value::Set constant parser reference-site oracle.

Genuine CALL rel32 destination sites and raw rodata literal-reference
addresses are read directly from the archived 2004.1 ELF. The current
experimental candidate has only 13 resolved relocations; the original
function contains 15 references. Avoid calling that candidate fully
relocation-equivalent until the two merged-away call sites are recovered.
"""
import collections
import importlib.util
import struct
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "vuc_decomp_value_set_sites", ROOT / "vuc/scripts/decomp.py")
decomp = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(decomp)

SET_ADDR = 0x0806e7d0
SET_SIZE = 455
ORIGINAL_CALL_SITES = {
    0x0806e82d: 0x08071800,  # MLGameString::c_str(), integer
    0x0806e843: 0x0804984c,  # __strtol_internal(), integer
    0x0806e864: 0x08071800,  # MLGameString::c_str(), vector
    0x0806e881: 0x080498dc,  # strrchr(), vector delimiter
    0x0806e899: 0x080498ac,  # memcpy(), vector scratch
    0x0806e8b2: 0x080495fc,  # __strtod_internal(), vector loop
    0x0806e8d9: 0x080495fc,  # __strtod_internal(), vector tail
    0x0806e8f0: 0x08071800,  # MLGameString::c_str(), scalar
    0x0806e900: 0x080495fc,  # __strtod_internal(), scalar
    0x0806e916: 0x08071800,  # MLGameString::c_str(), true comparison
    0x0806e940: 0x08071800,  # MLGameString::c_str(), false comparison
    0x0806e96f: 0x08071800,  # MLGameString::c_str(), boolean fallback
    0x0806e987: 0x0804984c,  # __strtol_internal(), boolean fallback
}
ORIGINAL_LITERAL_SITES = {
    0x0806e91c: (0x08075f78, b"true\x00"),
    0x0806e946: (0x0807668a, b"false\x00"),
}


class ValueConstvalReferences(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.elf = decomp.load()
        cls.sections = {s["name"]: s for s in cls.elf["sections"]}

    @classmethod
    def elf_bytes(cls, address, size, section):
        span = cls.sections[section]
        offset = address - span["address"]
        assert 0 <= offset <= span["size"] - size
        return cls.raw[span["offset"] + offset:span["offset"] + offset + size]

    def test_original_13_call_sites_and_destinations(self):
        self.assertEqual(len(ORIGINAL_CALL_SITES), 13)
        expected_counts = {
            0x08071800: 6, 0x0804984c: 2,
            0x080495fc: 3, 0x080498dc: 1, 0x080498ac: 1,
        }
        self.assertEqual(dict(collections.Counter(ORIGINAL_CALL_SITES.values())),
                         expected_counts)
        for address, target in ORIGINAL_CALL_SITES.items():
            with self.subTest(address=hex(address)):
                self.assertGreaterEqual(address, SET_ADDR)
                self.assertLess(address + 5, SET_ADDR + SET_SIZE + 1)
                opcode, displacement = struct.unpack(
                    "<Bi", self.elf_bytes(address, 5, ".text"))
                self.assertEqual(opcode, 0xe8)
                self.assertEqual(address + 5 + displacement, target)

    def test_original_boolean_literal_addresses_and_bytes(self):
        self.assertEqual(len(ORIGINAL_LITERAL_SITES), 2)
        for address, (target, literal) in ORIGINAL_LITERAL_SITES.items():
            with self.subTest(address=hex(address)):
                encoded = self.elf_bytes(address, 5, ".text")
                self.assertEqual(encoded[0], 0xba)  # mov edx, imm32
                self.assertEqual(struct.unpack_from("<I", encoded, 1)[0], target)
                self.assertEqual(self.elf_bytes(target, len(literal), ".rodata"),
                                 literal)

    def test_original_has_15_references_not_13(self):
        # The 13 sites currently resolved in the experimental candidate
        # must not be mistaken for the whole original's 13 call sites
        # plus two independent original string-address references.
        self.assertEqual(
            len(ORIGINAL_CALL_SITES) + len(ORIGINAL_LITERAL_SITES), 15)

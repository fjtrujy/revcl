"""Ground unmatched VUC control-flow bytes directly in the archived ELF.

These are original-code facts, not candidate acceptance or guessed source
syntax. The tests guard future source-level investigations of two of the
smallest nonexact functions without padding or manufactured instructions.
"""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "vuc_original_branch_layout", ROOT/"vuc/scripts/decomp.py")
decomp = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(decomp)


class NonexactOriginalBranchLayout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.elf = decomp.load()
        cls.text = next(s for s in cls.elf["sections"] if s["name"] == ".text")

    @classmethod
    def original(cls, address, length):
        offset = address - cls.text["address"]
        assert 0 <= offset <= cls.text["size"] - length
        return cls.raw[cls.text["offset"] + offset:
                       cls.text["offset"] + offset + length]

    def test_instruction_arena_cleanup_original_epilogue_pop_ecx(self):
        original = self.original(0x08051980, 46)
        self.assertEqual(len(original), 46)
        self.assertEqual(original[42:46], bytes.fromhex("59 5b 5d c3"))
        self.assertEqual(original[0:7], bytes.fromhex("55 89 e5 53 83 ec 04"))

    def test_value_addition_original_four_lane_order_and_scalar_alignment(self):
        original = self.original(0x0806e9b0, 219)
        # Four independent real x87 arithmetic instructions, in ascending
        # original vector-lane order 0,1,2,3.
        for offset, source_disp, target_disp in (
                (0xa7, 0xe8, 0x10),
                (0xb0, 0xec, 0x14),
                (0xb9, 0xf0, 0x18),
                (0xc2, 0xf4, 0x1c)):
            with self.subTest(offset=offset):
                self.assertEqual(
                    original[offset:offset + 9],
                    bytes([0xd9, 0x45, source_disp,
                           0xd8, 0x43, target_disp,
                           0xd9, 0x5b, target_disp]))
        # This three-byte NOP is the real original assembler-emitted
        # branch alignment, not license to place source-code padding.
        self.assertEqual(original[0xcd:0xd0],
                         bytes.fromhex("8d 76 00"))
        self.assertEqual(original[0xd0:0xd3],
                         bytes.fromhex("d9 45 e8"))

    def test_scope_normal_iterator_original_size_and_result_storage(self):
        original = self.original(0x0806a840, 149)
        self.assertEqual(len(original), 149)
        # Original i386 sret pointer lives in ESI and receives result
        # iterator's raw pointer after the fourth argument is loaded.
        self.assertEqual(original[0x12:0x1b],
                         bytes.fromhex("8b 55 14 39 fb 89 d8 89 16"))

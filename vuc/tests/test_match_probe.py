"""Strict VUC ELF32 relocation and function-byte comparison tests."""
import importlib.util
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "vuc/scripts/match_probe.py"
spec = importlib.util.spec_from_file_location("vuc_match_probe", SCRIPT)
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)


class MatchProbeTests(unittest.TestCase):
    def test_i386_absolute_relocation(self):
        self.assertEqual(match.relocate_word(1, 0x080720a0, 0, 0x08049b42), 0x080720a0)

    def test_i386_pc_relative_relocation(self):
        value = match.relocate_word(2, 0x080497bc, -4, 0x08049b56)
        self.assertEqual(struct.pack("<I", value), bytes.fromhex("62fcffff"))

    def test_unknown_relocation_rejected(self):
        with self.assertRaisesRegex(ValueError, "unsupported relocation"):
            match.relocate_word(255, 0, 0, 0)

    def test_exact_and_mutated_function(self):
        original = bytes.fromhex("5589e55dc3")
        self.assertTrue(match.compare(original, original)["exact"])
        mutated = bytearray(original)
        mutated[2] ^= 1
        result = match.compare(bytes(mutated), original)
        self.assertFalse(result["exact"])
        self.assertEqual(result["exact_bytes"], 4)
        self.assertEqual(result["different_offsets"], [2])
        with self.assertRaisesRegex(ValueError, "lengths differ"):
            match.compare(original[:-1], original)

    def test_probe_function_exists_in_archive(self):
        _, target = match.decomp.load()
        original = next(f for f in target["functions"] if f["address"] == 0x08049b40)
        self.assertEqual(original["size"], 30)


if __name__ == "__main__":
    unittest.main()

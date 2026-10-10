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
    def test_separate_original_literal_addresses_are_byte_authenticated(self):
        # One compiler object concatenates three C strings; the original ELF
        # linker placed each independently. No section-wide rebasing is valid.
        candidate = b"(\0, \0)\0"
        original = b"xxxx(\0yy, \0zzz)\0"
        original_section = {"offset": 0, "address": 0x1000, "size": len(original)}
        mapping = [{"offset": 0, "size": 2, "address": "0x1004"},
                   {"offset": 2, "size": 3, "address": "0x1008"},
                   {"offset": 5, "size": 2, "address": "0x100e"}]
        self.assertEqual(match.validate_literal_ranges(candidate, original,
                                                        original_section, mapping), 7)
        self.assertEqual(match.resolve_literal_range(mapping, 3), 0x1009)
        self.assertEqual(match.resolve_literal_range(mapping, 5), 0x100e)

        with self.assertRaisesRegex(ValueError, "one mapped range"):
            match.resolve_literal_range(mapping, 7)
        with self.assertRaisesRegex(ValueError, "unmapped bytes"):
            match.validate_literal_ranges(candidate, original, original_section, mapping[:-1])
        with self.assertRaisesRegex(ValueError, "overlapping"):
            match.validate_literal_ranges(candidate, original, original_section,
                                          mapping + [mapping[0]])
        with self.assertRaisesRegex(ValueError, "bytes differ"):
            match.validate_literal_ranges(b"!" + candidate[1:], original,
                                          original_section, mapping)
        with self.assertRaisesRegex(ValueError, "outside original"):
            broken = [dict(mapping[0], address="0x2000"), *mapping[1:]]
            match.validate_literal_ranges(candidate, original, original_section, broken)

    def test_named_bss_object_rebase(self):
        guard = (0, 0, 8, 1, 0, 4)
        instance = (0, 0x20, 0x2fc4, 1, 0, 4)
        symbols = [("guard", guard), ("instance", instance)]
        mapping = {"guard": "0x0808bce8", "instance": "0x0808c520"}
        self.assertEqual(match.resolve_object_target(symbols, 4, 0, mapping), 0x0808bce8)
        self.assertEqual(match.resolve_object_target(symbols, 4, 0x20, mapping), 0x0808c520)
        self.assertEqual(match.resolve_object_target(symbols, 4, 0x20 + 0x1fbf, mapping),
                         0x0808e4df)
        self.assertEqual(match.resolve_object_target(symbols, 4, 0x20 + 0x2fc0, mapping),
                         0x0808f4e0)

    def test_named_bss_object_rebase_must_be_unique_and_known(self):
        first = (0, 0, 8, 1, 0, 4)
        with self.assertRaisesRegex(ValueError, "one named object"):
            match.resolve_object_target([("a", first)], 4, 0x20, {"a": "0x123"})
        with self.assertRaisesRegex(ValueError, "unmapped original object"):
            match.resolve_object_target([("a", first)], 4, 0, {})
        with self.assertRaisesRegex(ValueError, "one named object"):
            match.resolve_object_target([("a", first), ("b", first)], 4, 0,
                                        {"a": "0x123", "b": "0x123"})

    def test_object_rebasing_uses_original_elf_symbols(self):
        raw, target = match.decomp.load()
        pairs = {
            ".bss": {
                "_ZGVZN16MLTempStringData3GetEvE3tsd": "0x0808bce8",
                "_ZZN16MLTempStringData3GetEvE3tsd": "0x0808c520",
            }
        }
        match.verify_original_objects(raw, target, pairs)
        incorrect = {".bss": {"_ZZN16MLTempStringData3GetEvE3tsd": "0x0808c524"}}
        with self.assertRaisesRegex(ValueError, "symbol mismatch"):
            match.verify_original_objects(raw, target, incorrect)
    def test_i386_absolute_relocation(self):
        self.assertEqual(match.relocate_word(1, 0x080720a0, 0, 0x08049b42), 0x080720a0)

    def test_local_defined_function_uses_original_base_not_candidate_offset(self):
        # ELF32 STT_FUNC, local, defined section 2, at object byte 0x130.
        local_function = (0, 0x130, 80, 2, 0, 2)
        section = (0, 0x130, 0, 3, 0, 2)
        self.assertEqual(match.linked_symbol_offset(local_function, 10), 0)
        self.assertEqual(match.linked_symbol_offset(section, 10), 0x130)
        # An undefined function has no candidate section and keeps its value.
        undefined = (0, 0, 0, 18, 0, 0)
        self.assertEqual(match.linked_symbol_offset(undefined, 10), 0)

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

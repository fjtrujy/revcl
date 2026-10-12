"""Original VUC linker-layout reference must be authenticated and self-consistent."""
import importlib.util
import json
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_link_layout", ROOT / "vuc/scripts/original_link_layout.py")
sys.path.insert(0, str(ROOT / "vuc/scripts"))
layout = importlib.util.module_from_spec(spec)
spec.loader.exec_module(layout)


class OriginalLinkLayoutTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.info = layout.decomp.load()
        cls.report = layout.parse_layout(cls.raw, cls.info)

    def test_original_elf_segments_and_metadata_are_pinned(self):
        report = self.report
        self.assertEqual(report["original_file_size"], 314338)
        self.assertEqual(report["elf_header"]["entry"], 0x08049A20)
        self.assertEqual(len(report["program_headers"]), 8)
        self.assertEqual(len(report["sections"]), 35)
        self.assertEqual([p["index"] for p in report["program_headers"]
                          if p["type"] == "LOAD"], [2, 3])
        self.assertEqual(report["original_sha256"], layout.decomp.CANONICAL_SHA256)

    def test_bss_is_allocated_but_not_file_backed(self):
        bss = next(x for x in self.report["sections"] if x["name"] == ".bss")
        self.assertFalse(bss["file_backed"])
        self.assertEqual(bss["size"], 19780)
        self.assertEqual(bss["load_segment_index"], 3)
        self.assertEqual(bss["file_offset"], 0x427A0)
        self.assertFalse(self.report["whole_elf_byte_exact"])
        self.assertIsNone(self.report["matching_executable_file_bytes"])

    def test_original_dynamic_relocations_are_independently_read(self):
        records = self.report["dynamic_relocations"]
        self.assertEqual(len(records), 82)
        self.assertEqual(sum(x["relocation_section"] == ".rel.dyn" for x in records), 15)
        self.assertEqual(sum(x["relocation_section"] == ".rel.plt" for x in records), 67)
        self.assertEqual(sum(x["type"] == 7 for x in records), 67)
        self.assertEqual(sum(x["type"] == 5 for x in records), 14)
        self.assertEqual(sum(x["type"] == 6 for x in records), 1)
        self.assertEqual(records[0]["dynamic_symbol_name"], "__gmon_start__")
        self.assertEqual(records[1]["dynamic_symbol_name"], "_ZSt4cout")

    def test_candidate_layout_is_parsed_without_original_section_inventory(self):
        candidate = layout.inspect_candidate(self.raw)
        comparison = layout.compare_link_layout(self.report, candidate)
        self.assertTrue(comparison["layout_equal"])
        self.assertEqual(comparison["difference_count"], 0)
        self.assertIsNone(comparison["whole_elf_byte_exact"])
        self.assertIsNone(comparison["verified_instruction_or_data_bytes"])
        self.assertEqual(candidate["sections"][12]["name"], ".text")

    def test_candidate_with_changed_instruction_still_has_same_layout(self):
        # This deliberately demonstrates why matching layout is NOT byte equality.
        candidate = bytearray(self.raw)
        text = next(s for s in self.info["sections"] if s["name"] == ".text")
        candidate[text["offset"] + 30] ^= 1
        comparison = layout.compare_link_layout(
            self.report, layout.inspect_candidate(candidate))
        self.assertTrue(comparison["layout_equal"])
        self.assertIsNone(comparison["whole_elf_byte_exact"])

    def test_candidate_load_permission_differences_are_reported(self):
        candidate = bytearray(self.raw)
        header = layout.decomp.ELF_HEADER.unpack_from(candidate)
        index = header[5] + 2 * header[9]
        struct.pack_into("<I", candidate, index + 24, 7)
        comparison = layout.compare_link_layout(
            self.report, layout.inspect_candidate(candidate))
        self.assertFalse(comparison["layout_equal"])
        self.assertEqual(comparison["difference_count"], 1)
        self.assertEqual(comparison["differences"][0]["field"], "flags")
        self.assertEqual(comparison["differences"][0]["category"], "program_headers")

    def test_invalid_candidate_section_name_table_rejected(self):
        candidate = bytearray(self.raw)
        header = layout.decomp.ELF_HEADER.unpack_from(candidate)
        string_index = header[13]
        string_entry = header[6] + header[11] * string_index
        struct.pack_into("<I", candidate, string_entry + 16, len(candidate) - 1)
        with self.assertRaisesRegex(ValueError, "section names out of bounds"):
            layout.inspect_candidate(candidate)

    def test_dynamic_relocation_target_outside_load_is_rejected(self):
        original = bytearray(self.raw)
        section = next(x for x in self.info["sections"] if x["name"] == ".rel.plt")
        struct.pack_into("<I", original, section["offset"], 0x12345678)
        with self.assertRaisesRegex(ValueError, "relocation target outside LOAD"):
            layout.parse_layout(original, self.info)

    def test_dynamic_relocation_symbol_index_out_of_range_is_rejected(self):
        original = bytearray(self.raw)
        section = next(x for x in self.info["sections"] if x["name"] == ".rel.plt")
        struct.pack_into("<I", original, section["offset"] + 4, 0xFFFFF07)
        with self.assertRaisesRegex(ValueError, "refers beyond DYNSYM"):
            layout.parse_layout(original, self.info)

    def test_inter_section_file_holes_are_not_attributed_to_code(self):
        exec_segment = self.report["program_headers"][2]
        self.assertGreater(len(exec_segment["non_section_file_ranges"]), 0)
        gap = exec_segment["non_section_file_ranges"][0]
        self.assertEqual(gap["file_offset"], 0)
        self.assertGreaterEqual(gap["size"], 52 + 8 * 32)

    def test_repeated_covered_ranges_do_not_invent_negative_gaps(self):
        self.assertEqual(layout.subtract_intervals(
            0, 100, [(10, 20), (15, 40), (60, 90)]),
            [{"file_offset": 0, "size": 10},
             {"file_offset": 40, "size": 20},
             {"file_offset": 90, "size": 10}])

    def test_load_segment_larger_than_its_memory_range_rejected(self):
        original = bytearray(self.raw)
        header = layout.decomp.ELF_HEADER.unpack_from(original)
        third = header[5] + 2 * header[9]
        struct.pack_into("<I", original, third + 16, 0x400000)
        with self.assertRaisesRegex(ValueError, "file bytes out of bounds"):
            layout.parse_layout(original, self.info)

    def test_misaligned_load_rejected(self):
        original = bytearray(self.raw)
        header = layout.decomp.ELF_HEADER.unpack_from(original)
        third = header[5] + 2 * header[9]
        struct.pack_into("<I", original, third + 4, 1)
        with self.assertRaisesRegex(ValueError, "virtual/file alignment"):
            layout.parse_layout(original, self.info)

    def test_unmapped_original_allocated_section_rejected(self):
        original = bytearray(self.raw)
        header = layout.decomp.ELF_HEADER.unpack_from(original)
        section = next(s for s in self.info["sections"] if s["name"] == ".text")
        index = section["index"]
        struct.pack_into("<I", original, header[6] + index * header[11] + 12,
                         0x02000000)
        with self.assertRaisesRegex(ValueError, "inventory and header disagree"):
            layout.parse_layout(original, self.info)

    def test_committed_reference_matches_rederived_original(self):
        reference = json.loads(layout.DEFAULT.read_text())
        self.assertEqual(reference, self.report)


if __name__ == "__main__":
    unittest.main()

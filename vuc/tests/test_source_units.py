"""ELF translation-unit grouping is evidence, not inferred global ownership."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_source_units", ROOT / "vuc/scripts/source_units.py")
units = importlib.util.module_from_spec(spec)
spec.loader.exec_module(units)


class SourceUnitTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        raw, info = units.decomp.load()
        cls.records, cls.all_files = units.recover(info, raw)

    def test_original_translation_units(self):
        self.assertEqual(self.all_files, 57)
        self.assertEqual(len(self.records), 22)
        self.assertEqual(len({rec["file"] for rec in self.records}), 22)
        self.assertIn("vuc_compiler.cpp", {rec["file"] for rec in self.records})

    def test_known_locals_are_attached_to_source_records(self):
        by_name = {r["file"]: r for r in self.records}
        addresses = {fn["address"] for fn in by_name["Instruction.cpp"]["local_functions"]}
        self.assertIn(0x080514a0, addresses)
        self.assertNotIn(0x08051900, addresses)  # Global: no local-file attribution
        self.assertEqual(sum(len(r["local_functions"]) for r in self.records), 79)

    def test_checked_in_inventory_matches_canonical_archive(self):
        self.assertEqual(units.OUTPUT.read_text(), units.tsv(self.records))


if __name__ == "__main__":
    unittest.main()

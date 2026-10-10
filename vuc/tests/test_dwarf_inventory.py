"""Validate surviving DWARF provenance without inventing application debug types."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_dwarf_inventory",
                                              ROOT / "vuc/scripts/dwarf_inventory.py")
dwarf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dwarf)


class DwarfInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        raw, target = dwarf.decomp.load()
        cls.units = dwarf.inventory(raw, target)

    def test_only_runtime_startup_compilation_units_survive(self):
        self.assertEqual(len(self.units), 4)
        self.assertEqual([Path(u["name"]).name for u in self.units],
                         ["start.S", "init.c", "crti.S", "crtn.S"])
        self.assertTrue(all("glibc-2.3.2" in u["comp_dir"]
                            for u in self.units))

    def test_runtime_assembler_and_compiler_provenance(self):
        self.assertEqual({u["producer"] for u in self.units},
                         {"GNU AS 2.14.90.0.7",
                          "GNU C 3.3.3 20031206 (prerelease) (Debian)"})
        self.assertEqual(self.units[0]["low_pc"], 0x08049a20)
        self.assertEqual(self.units[0]["high_pc"], 0x08049a42)

    def test_checked_in_evidence_matches_canonical_dwarf(self):
        saved = json.loads(dwarf.OUTPUT.read_text())
        self.assertEqual(saved["compilation_units"], 4)
        self.assertEqual(saved["application_units"], [])
        self.assertEqual(saved["units"], self.units)

    def test_truncated_leb_rejected(self):
        with self.assertRaisesRegex(ValueError, "truncated"):
            dwarf.leb(b"\x80", 0)


if __name__ == "__main__":
    unittest.main()

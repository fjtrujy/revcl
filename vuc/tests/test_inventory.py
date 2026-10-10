"""Checks for the VUC archive-backed ELF inventory, without compiler dependencies."""
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "vuc/scripts/decomp.py"
spec = importlib.util.spec_from_file_location("vuc_decomp", SCRIPT)
decomp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decomp)


class InventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data, cls.info = decomp.load()

    def test_canonical_archive_identity(self):
        self.assertEqual(len(self.data), 314338)
        self.assertEqual(self.info["sha256"], decomp.CANONICAL_SHA256)
        self.assertEqual(self.info["elf"]["machine"], "i386")

    def test_structural_inventory(self):
        self.assertEqual(len(self.info["sections"]), 35)
        self.assertEqual(len(self.info["symbols"]), 716)
        self.assertEqual(len(self.info["functions"]), 298)
        self.assertEqual(self.info["text"]["size"], 165440)
        self.assertTrue(any(s["name"] == ".debug_info" for s in self.info["sections"]))

    def test_symbol_lookup(self):
        symbol = "_Z23AsmOutput_WritePrologueRSt14basic_ofstreamIcSt11char_traitsIcEE"
        by_name = decomp.find_function(self.info, symbol)
        self.assertEqual(by_name["address"], 0x08049b40)
        self.assertEqual(decomp.find_function(self.info, "0x08049b40"), by_name)
        self.assertEqual(decomp.find_function(self.info, "0x08049b41"), by_name)

    def test_checked_in_baseline(self):
        baseline = json.loads((ROOT / "vuc/config/target.json").read_text())
        self.assertEqual(baseline["sha256"], self.info["sha256"])
        self.assertEqual(baseline["function_symbol_count"], len(self.info["functions"]))
        lines = (ROOT / "vuc/config/functions.tsv").read_text().splitlines()
        self.assertEqual(len(lines), 1 + len(self.info["functions"]))
        self.assertEqual(lines[0], "address\tsize\tbinding\tname")

    def test_status_never_fabricates_score(self):
        result = subprocess.run([sys.executable, str(SCRIPT), "status"],
                                text=True, capture_output=True, check=True)
        self.assertIn("UNAVAILABLE", result.stdout)
        self.assertNotIn("100%", result.stdout)


if __name__ == "__main__":
    unittest.main()

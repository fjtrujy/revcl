"""Deterministic, conservative original-ELF named-data gap inventory."""
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_data_gaps", ROOT/"vuc/scripts/data_gap_inventory.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class DataGapInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.report = module.inventory()

    def test_only_explicitly_accepted_data_objects_count_as_exact(self):
        self.assertEqual(self.report["named_data_symbols"], 334)
        self.assertEqual(self.report["exact_data_symbols"], 244)
        self.assertEqual(self.report["unresolved_data_symbols"], 90)
        self.assertIsNone(self.report["whole_binary_score"])

    def test_section_counts_are_not_a_whole_binary_data_coverage_claim(self):
        self.assertEqual(self.report["unresolved_by_section"],
                         {".bss": 66, ".data": 2, ".rodata": 22})
        self.assertEqual(sum(self.report["unresolved_by_section"].values()), 90)

    def test_two_matching_type_name_probes_remain_unaccepted(self):
        outstanding = {
            (r["symbol"], r["address"]): r
            for r in self.report["unresolved"]
        }
        self.assertEqual(
            outstanding[("_ZZrsRSiR4TypeE2tn", "0x0807700c")]["size"], 28)
        self.assertEqual(
            outstanding[("_ZZlsRSo4TypeE2tn", "0x08077b60")]["size"], 28)
        self.assertTrue(all(r["status"] == "UNRESOLVED"
                            for r in self.report["unresolved"]))

    def test_probe_configs_cannot_silently_enter_the_exact_manifest(self):
        import json
        exact = set(json.loads(
            (ROOT/"vuc/config/exact_data_candidates.json").read_text()))
        for name in ("input", "output"):
            probe = "vuc/config/type_stream_%s_names_oct12_probe.json" % name
            self.assertNotIn(probe, exact)
            record = json.loads((ROOT/probe).read_text())
            self.assertTrue(record["source"].startswith("vuc/probes/"))


if __name__ == "__main__":
    unittest.main()

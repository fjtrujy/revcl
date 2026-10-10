"""Every original VUC function must have an explicit reconstruction status."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_coverage", ROOT / "vuc/scripts/coverage.py")
coverage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(coverage)


class CoverageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        _, info = coverage.decomp.load()
        cls.info = info
        cls.exact = json.loads((coverage.CONFIG / "exact_candidates.json").read_text())
        cls.exploratory = json.loads((coverage.CONFIG / "selected_candidates.json").read_text())
        cls.rows = coverage.classify(info, cls.exact, cls.exploratory)

    def test_every_original_function_is_accounted_for(self):
        self.assertEqual(len(self.rows), 298)
        self.assertEqual(sum(r[2] == "EXACT" for r in self.rows), 97)
        self.assertEqual(sum(r[2] == "EXPERIMENTAL" for r in self.rows), 8)
        self.assertEqual(sum(r[2] == "UNRESOLVED" for r in self.rows), 193)
        self.assertEqual(coverage.OUTPUT.read_text(), coverage.table(self.rows))

    def test_complete_mlgamestring_member_cluster_is_exact(self):
        # The Itanium C++ mangled class-name prefix excludes MLGameStringData,
        # MLGameStringTable, MLTempString and hash-table template functions.
        methods = [r for r in self.rows if r[5].startswith(("_ZN12MLGameString", "_ZNK12MLGameString"))]
        self.assertEqual(len(methods), 9)
        self.assertTrue(all(r[2] == "EXACT" for r in methods))

    def test_partial_prologue_is_never_accepted(self):
        prologue = next(r for r in self.rows if r[0] == 0x08049b40)
        self.assertEqual(prologue[2], "EXPERIMENTAL")

    def test_duplicate_exact_manifest_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "duplicate"):
            coverage.classify(self.info, self.exact + self.exact[:1], self.exploratory)

    def test_invalid_manifest_is_rejected(self):
        with self.assertRaises((ValueError, OSError)):
            coverage.classify(self.info, ["vuc/config/does-not-exist.json"], [])


if __name__ == "__main__":
    unittest.main()

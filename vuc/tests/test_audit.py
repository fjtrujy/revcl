"""Candidate audit scope, totals, and duplicate detection."""
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_audit", ROOT / "vuc/scripts/audit.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class AuditTests(unittest.TestCase):
    def test_explicit_scope_and_totals(self):
        cases = [
            {"function": "one", "length": 30, "exact_bytes": 22,
             "exact": False, "verified_literal_bytes": 92},
            {"function": "two", "length": 10, "exact_bytes": 10,
             "exact": True, "verified_literal_bytes": 0},
        ]
        with patch.object(audit.matcher, "run", side_effect=cases):
            result = audit.audit([{"config": "a", "object": "a.o"},
                                  {"config": "b", "object": "b.o"}])
        self.assertIsNone(result["whole_binary_score"])
        self.assertEqual(result["checked_code_bytes"], 40)
        self.assertEqual(result["matching_code_bytes"], 32)
        self.assertEqual(result["checked_literal_bytes"], 92)
        self.assertEqual(result["fully_exact_functions"], 1)
        self.assertEqual(result["functions_checked"], 2)

    def test_duplicates_rejected(self):
        sample = {"function": "same", "length": 1, "exact_bytes": 1,
                  "exact": True, "verified_literal_bytes": 0}
        with patch.object(audit.matcher, "run", return_value=sample):
            with self.assertRaisesRegex(ValueError, "duplicate"):
                audit.audit([{"config": "a", "object": "a.o"},
                             {"config": "b", "object": "a.o"}])

    def test_empty_manifest_rejected(self):
        with self.assertRaises(ValueError):
            audit.audit([])


if __name__ == "__main__":
    unittest.main()

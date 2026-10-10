"""Optimization-sweep reporting must never claim whole-binary fidelity."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_variants", ROOT / "vuc/scripts/compare_variants.py")
variants = importlib.util.module_from_spec(spec)
spec.loader.exec_module(variants)


class VariantTests(unittest.TestCase):
    def test_candidate_results_and_errors(self):
        class Result:
            returncode = 0
            stderr = ""

        with tempfile.TemporaryDirectory() as directory:
            with patch.object(variants.subprocess, "run", return_value=Result()), \
                    patch.object(variants.matcher, "run",
                                 return_value={"length": 30, "exact_bytes": 22, "exact": False}):
                report = variants.run("compiler", directory, "source.cpp", "config.json",
                                      {"O2": ["-O2"]})
            self.assertIsNone(report["whole_binary_score"])
            self.assertEqual(report["results"]["O2"]["exact_bytes"], 22)
            self.assertTrue((Path(directory) / "comparison.json").exists())

    def test_failed_compile_is_not_reported_as_match(self):
        class Result:
            returncode = 1
            stderr = "compiler error"

        with tempfile.TemporaryDirectory() as directory:
            with patch.object(variants.subprocess, "run", return_value=Result()):
                report = variants.run("compiler", directory, "source.cpp", "config.json",
                                      {"invalid": ["-not-a-real-flag"]})
            record = report["results"]["invalid"]
            self.assertEqual(record["compile_exit"], 1)
            self.assertNotIn("exact_bytes", record)
            self.assertIn("compiler error", record["error"])


if __name__ == "__main__":
    unittest.main()

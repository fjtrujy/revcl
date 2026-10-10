"""Candidate-suite scope, compilation grouping and error handling."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_candidate_suite", ROOT / "vuc/scripts/match_candidates.py")
suite = importlib.util.module_from_spec(spec)
spec.loader.exec_module(suite)


class CandidateSuiteTests(unittest.TestCase):
    def test_baseline_manifest_has_unique_functions(self):
        configs = json.loads(suite.DEFAULT_MANIFEST.read_text())
        names = [json.loads(suite.within_repo(p).read_text())["function"] for p in configs]
        self.assertEqual(len(names), 4)
        self.assertEqual(len(set(names)), len(names))

    def test_rejects_path_escape(self):
        with self.assertRaisesRegex(ValueError, "escapes repository"):
            suite.within_repo("../outside")

    def test_aggregates_only_checked_functions(self):
        configs = ["vuc/config/list_file_lines_probe.json",
                   "vuc/config/ml_game_string_cstr_probe.json"]
        results = [
            {"function": "ListFile", "exact": True, "exact_bytes": 18,
             "length": 18, "verified_literal_bytes": 0},
            {"function": "MLGameString", "exact": False, "exact_bytes": 10,
             "length": 13, "verified_literal_bytes": 0},
        ]
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as compile_mock:
                with patch.object(suite.matcher, "run", side_effect=results):
                    summary = suite.run("compiler", out, configs)
            self.assertEqual(compile_mock.call_count, 1)
            self.assertEqual(summary["functions_checked"], 2)
            self.assertEqual(summary["fully_exact_functions"], 1)
            self.assertEqual(summary["matching_code_bytes"], 28)
            self.assertEqual(summary["checked_code_bytes"], 31)
            self.assertIsNone(summary["whole_binary_score"])
            self.assertTrue((Path(out) / "candidate_summary.json").is_file())

    def test_compiler_failure_is_not_a_match(self):
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 1, "", "compile error")):
                with self.assertRaisesRegex(RuntimeError, "compile error"):
                    suite.run("compiler", out, ["vuc/config/list_file_lines_probe.json"])
            self.assertFalse((Path(out) / "candidate_summary.json").exists())

    def test_duplicate_function_rejected(self):
        config = "vuc/config/list_file_lines_probe.json"
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run",
                                  return_value={"exact": True, "exact_bytes": 18,
                                                "length": 18, "verified_literal_bytes": 0}):
                    with self.assertRaisesRegex(ValueError, "duplicate function"):
                        suite.run("compiler", out, [config, config])


if __name__ == "__main__":
    unittest.main()

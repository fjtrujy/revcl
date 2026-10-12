"""Tests for VUC's independent, strict named initialized-data acceptance gate."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_match_data_candidates", ROOT/"vuc/scripts/match_data_candidates.py")
suite = importlib.util.module_from_spec(spec)
spec.loader.exec_module(suite)


class NamedDataCandidateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.configs = json.loads(suite.EXACT_MANIFEST.read_text())
        cls.canonical_raw, cls.canonical_target = suite.matcher.match.decomp.load()

    def test_six_original_independent_data_objects(self):
        self.assertEqual(len(self.configs), 6)
        records = [json.loads(suite.within_repo(name).read_text()) for name in self.configs]
        self.assertEqual(len({(r["symbol"], r["address"]) for r in records}), 6)
        self.assertEqual(sum(r["size"] for r in records), 68352)
        for r in records:
            self.assertEqual(r["section"], ".data")
            self.assertEqual(r["relocations"], {})
            self.assertEqual(r["source"], "vuc/src/GlobalBuffers.cpp")
            self.assertIn("-fno-zero-initialized-in-bss", r["flags"])
            addr, length, section_id, name = suite.matcher.original_symbol(
                self.canonical_raw, self.canonical_target,
                r["symbol"], r["address"], r["size"], 1)
            self.assertEqual(addr, int(r["address"], 0))
            self.assertEqual(length, r["size"])
            self.assertEqual(name, ".data")
            sec = self.canonical_target["sections"][section_id]
            begin = sec["offset"] + addr - sec["address"]
            self.assertEqual(self.canonical_raw[begin:begin + length],
                             bytes(length), r["symbol"])

    def test_all_six_are_built_once_and_must_be_exact(self):
        def matched(obj, config):
            record = json.loads(Path(config).read_text())
            return {"symbol": record["symbol"], "size": record["size"],
                    "exact_bytes": record["size"], "exact": True,
                    "resolved_relocations": 0}
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as compiler:
                with patch.object(suite.matcher, "run", side_effect=matched):
                    result = suite.run("compiler", folder, self.configs)
            self.assertEqual(compiler.call_count, 1)
            self.assertEqual(result["data_objects_checked"], 6)
            self.assertEqual(result["fully_exact_data_objects"], 6)
            self.assertEqual(result["checked_data_bytes"], 68352)
            self.assertEqual(result["matching_data_bytes"], 68352)
            self.assertTrue(result["all_candidates_exact"])
            self.assertIsNone(result["whole_binary_score"])
            self.assertFalse(result["whole_binary_byte_exact"])
            self.assertTrue((Path(folder)/"data_candidate_summary.json").is_file())

    def test_partial_match_rejects_whole_set(self):
        def fail(obj, config):
            rec = json.loads(Path(config).read_text())
            return {"symbol": rec["symbol"], "size": rec["size"],
                    "exact_bytes": rec["size"]-1, "exact": False,
                    "resolved_relocations": 0}
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run", side_effect=fail):
                    result = suite.run("compiler", folder, self.configs)
            self.assertEqual(result["acceptance"], "FAIL")
            self.assertEqual(result["fully_exact_data_objects"], 0)
            self.assertEqual(result["matching_data_bytes"], 68352-6)

    def test_duplicate_and_escape_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run",
                                  return_value={"exact": True, "size": 1024,
                                                "exact_bytes": 1024,
                                                "resolved_relocations": 0}):
                    with self.assertRaisesRegex(ValueError, "duplicate original"):
                        suite.run("compiler", folder, [self.configs[0]] * 2)
            with self.assertRaisesRegex(ValueError, "escapes repository"):
                suite.run("compiler", folder, ["../../etc/passwd"])
            with self.assertRaisesRegex(ValueError, "nonempty"):
                suite.run("compiler", folder, [])


if __name__ == "__main__":
    unittest.main()

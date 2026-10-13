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
    def test_baseline_manifest_has_unique_original_symbol_addresses(self):
        configs = json.loads(suite.DEFAULT_MANIFEST.read_text())
        records = [json.loads(suite.within_repo(p).read_text()) for p in configs]
        self.assertEqual(len(records), 177)
        # Local function spelling may repeat across original C++ units.
        self.assertEqual(len({(r["function"], r["address"]) for r in records}), len(records))

    def test_exact_regression_set_is_subset_of_diagnostics(self):
        all_configs = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        exact_configs = json.loads(suite.EXACT_MANIFEST.read_text())
        self.assertEqual(len(exact_configs), 170)
        self.assertTrue(set(exact_configs).issubset(all_configs))
        for manifest in exact_configs:
            source = json.loads(suite.within_repo(manifest).read_text())["source"]
            self.assertTrue(source.startswith("vuc/src/"), source)
        for manifest in all_configs - set(exact_configs):
            source = json.loads(suite.within_repo(manifest).read_text())["source"]
            self.assertTrue(source.startswith("vuc/probes/"), source)

    def test_manifest_sources_and_configs_are_committed(self):
        """A passing local VM is insufficient if clean clones lack its inputs."""
        if not (ROOT / ".git").exists():
            self.skipTest("git working tree required for committed source audit")
        tracked = set(subprocess.check_output(
            ["git", "ls-files", "-z"], cwd=ROOT).decode().split("\0"))
        for filename in (suite.EXACT_MANIFEST, suite.DEFAULT_MANIFEST):
            for config_name in json.loads(filename.read_text()):
                with self.subTest(manifest=filename.name, config=config_name):
                    self.assertIn(config_name, tracked)
                    config = json.loads(suite.within_repo(config_name).read_text())
                    self.assertIn(config["source"], tracked)

    def test_singleton_cleanup_exact_does_not_promote_its_nonexact_getter(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        exploratory = set(json.loads(suite.DEFAULT_MANIFEST.read_text()))
        self.assertIn("vuc/config/ml_game_string_table_cleanup_probe.json", accepted)
        self.assertNotIn("vuc/config/ml_string_table_get_probe.json", accepted)
        self.assertIn("vuc/config/ml_string_table_get_probe.json", exploratory)

    def test_printer_unit_exactness_excludes_partial_ast_call(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for filename in ("ugly_printer_unit_oct13_probe.json",
                         "iostream_global_printer_oct13_probe.json"):
            config_path = "vuc/config/" + filename
            self.assertIn(config_path, accepted)
            config = json.loads(suite.within_repo(config_path).read_text())
            self.assertEqual(config["source"], "vuc/src/Printer.cpp")
        self.assertNotIn("vuc/config/print_ast_fun_call_probe.json", accepted)

    def test_ast_matchstack_initializer_and_wrapper_share_real_source(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for filename in ("ast_matchstack_init_oct13_probe.json",
                         "iostream_global_matchStack_probe.json"):
            path = "vuc/config/" + filename
            self.assertIn(path, accepted)
            self.assertEqual(json.loads(suite.within_repo(path).read_text())["source"],
                             "vuc/src/AST.cpp")
        helper = json.loads(suite.within_repo(
            "vuc/config/ast_matchstack_init_oct13_probe.json").read_text())
        self.assertEqual(helper["size"], 145)
        self.assertEqual(helper["relocation_objects"][".bss"]["matchStack"],
                         "0x0808abe4")
        self.assertEqual(helper["relocation_literals"][".rodata"][0]["size"], 14)

    def test_export_module_lifecycle_is_exact_but_reserve_is_not(self):
        accepted = set(json.loads(suite.EXACT_MANIFEST.read_text()))
        for name in ("scope_vector_teardown_probe.json",
                     "export_module_scope_wrapper_probe.json",
                     "export_module_static_initializer_probe.json"):
            self.assertIn("vuc/config/" + name, accepted)
        self.assertNotIn("vuc/config/scope_vector_reserve_probe.json", accepted)

    def test_original_tu_flag_hypothesis_is_explicit(self):
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as run_mock:
                with patch.object(suite.matcher, "run", return_value={
                    "function": "c_str", "exact": True, "exact_bytes": 13,
                    "length": 13, "verified_literal_bytes": 0}):
                    result = suite.run("compiler", out,
                                       ["vuc/config/ml_game_string_cstr_probe.json"])
            command = run_mock.call_args.args[0]
            self.assertIn("-frename-registers", command)
            self.assertEqual(result["results"][0]["candidate_flags"],
                             ["-O2", "-frename-registers"])

    def test_cli_fails_for_any_partial_function(self):
        result = {"all_candidates_exact": False, "acceptance": "FAIL",
                  "fully_exact_functions": 1, "functions_checked": 2,
                  "matching_code_bytes": 9, "checked_code_bytes": 18}
        with tempfile.TemporaryDirectory() as out:
            with patch("sys.argv", ["match_candidates.py", "--compiler", "mock-gcc",
                                    "--out", out]):
                with patch.object(suite, "run", return_value=result):
                    self.assertEqual(suite.main(), 1)

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
            self.assertEqual(compile_mock.call_count, 2)
            self.assertEqual(summary["functions_checked"], 2)
            self.assertEqual(summary["fully_exact_functions"], 1)
            self.assertEqual(summary["matching_code_bytes"], 28)
            self.assertEqual(summary["checked_code_bytes"], 31)
            self.assertIsNone(summary["whole_binary_score"])
            self.assertFalse(summary["all_candidates_exact"])
            self.assertEqual(summary["acceptance"], "FAIL")
            self.assertTrue((Path(out) / "candidate_summary.json").is_file())

    def test_exact_candidates_can_pass_only_without_partial_matches(self):
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run", return_value={
                    "function": "ListFile", "exact": True, "exact_bytes": 18,
                    "length": 18, "verified_literal_bytes": 0}):
                    summary = suite.run("compiler", out,
                                        ["vuc/config/list_file_lines_probe.json"])
            self.assertTrue(summary["all_candidates_exact"])
            self.assertEqual(summary["acceptance"], "PASS")
            self.assertIsNone(summary["whole_binary_score"])

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

    def test_duplicate_local_name_with_distinct_original_addresses_is_allowed(self):
        paths = ["vuc/config/iostream_cleanup_0804a6f0_probe.json",
                 "vuc/config/iostream_cleanup_0804a7c0_probe.json"]
        result = {"function": "__tcf_0", "exact": True, "exact_bytes": 16,
                  "length": 16, "verified_literal_bytes": 0}
        with tempfile.TemporaryDirectory() as out:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as comp:
                with patch.object(suite.matcher, "run", return_value=result):
                    summary = suite.run("compiler", out, paths)
        self.assertEqual(comp.call_count, 1)
        self.assertEqual(summary["functions_checked"], 2)
        self.assertEqual(summary["checked_code_bytes"], 32)
        self.assertTrue(summary["all_candidates_exact"])


if __name__ == "__main__":
    unittest.main()

"""Partial VUC functions are reproducible checkpoints, never exact acceptance."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_partial_checkpoints", ROOT / "vuc/scripts/match_partial_checkpoints.py")
partials = importlib.util.module_from_spec(spec)
spec.loader.exec_module(partials)


class PartialCheckpointTests(unittest.TestCase):
    def setUp(self):
        self.valid = {"manifest": "vuc/config/example.json",
                      "function": "_Z4Testv", "address": "0x08050000",
                      "original_size": 10, "status": "PARTIAL",
                      "exact_bytes": 9, "relocations": 2,
                      "verified_literal_bytes": 3}

    def test_only_full_length_relocation_verified_functions_become_checkpoints(self):
        invalid = dict(self.valid, manifest="vuc/config/invalid.json",
                       status="UNVERIFIED", error="function lengths differ: 8 vs 10")
        checkpoints = partials.checkpoint_rows([invalid, self.valid])
        self.assertEqual(list(checkpoints), [self.valid["manifest"]])
        self.assertEqual(checkpoints[self.valid["manifest"]]["exact_bytes_floor"], 9)
        self.assertEqual(checkpoints[self.valid["manifest"]]["relocations"], 2)
        self.assertEqual(checkpoints[self.valid["manifest"]]["verified_literal_bytes"], 3)

    def test_regression_blocks_degradation_or_loss(self):
        baseline = partials.checkpoint_rows([self.valid])
        improved = dict(self.valid, exact_bytes=10, status="EXACT_READY")
        self.assertEqual(partials.check_baseline([improved], baseline), [])
        degraded = dict(self.valid, exact_bytes=8)
        self.assertIn("fell below", partials.check_baseline([degraded], baseline)[0])
        lost = dict(self.valid, status="UNVERIFIED")
        self.assertIn("no longer valid", partials.check_baseline([lost], baseline)[0])
        self.assertIn("missing checkpoint", partials.check_baseline([], baseline)[0])
        renamed = dict(self.valid, address="0x08050001")
        self.assertIn("address changed", partials.check_baseline([renamed], baseline)[0])

    def test_improvement_requires_authenticated_original_offsets(self):
        baseline = partials.checkpoint_rows([self.valid])
        improved = dict(self.valid, exact_bytes=10, status="EXACT_READY")
        gains = partials.improvements([improved], baseline)
        self.assertEqual(gains[0]["improvement"], 1)
        rejected = dict(improved, status="UNVERIFIED", error="length mismatch")
        self.assertEqual(partials.improvements([rejected], baseline), [])
        self.assertEqual(partials.improvements([self.valid], baseline), [])

    def test_partial_manifest_is_exact_disjoint_and_keeps_sources_in_probes(self):
        accepted = set(partials.read_manifest(partials.EXACT))
        selected = partials.read_manifest(partials.SELECTED)
        open_entries = [path for path in selected if path not in accepted]
        self.assertEqual(len(accepted), 205)
        self.assertEqual(len(open_entries), 39)
        for path in open_entries:
            config = json.loads(partials.checked_path(path).read_text())
            self.assertTrue(config["source"].startswith("vuc/probes/"))
        self.assertNotIn("vuc/config/print_ast_lval_recursion_oct23_probe.json",
                         open_entries)  # its current compiled body is wrong-sized.

    def test_duplicate_and_escape_paths_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "duplicate"):
            partials.diagnostic("/unused/gcc", "/tmp/unused",
                                ["vuc/config/scope_vector_insert_partial_oct15i.json"] * 2,
                                set())
        with self.assertRaisesRegex(ValueError, "escapes repository"):
            partials.checked_path("../../../../etc/passwd")

    def test_summary_cannot_claim_whole_binary_or_exact_acceptance(self):
        summary = partials.summarize([self.valid], [])
        self.assertIsNone(summary["whole_binary_score"])
        self.assertFalse(summary["exact_acceptance"])
        self.assertEqual(summary["verified_partial_bytes"], 9)
        self.assertEqual(summary["verified_original_function_bytes"], 10)
        self.assertEqual(summary["verified_original_size_partials"], 1)


if __name__ == "__main__":
    unittest.main()

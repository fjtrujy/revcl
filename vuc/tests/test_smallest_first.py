"""Size-first work planning must not turn source guesses into accepted matches."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_smallest_first", ROOT / "vuc/scripts/smallest_first.py")
small = importlib.util.module_from_spec(spec)
spec.loader.exec_module(small)


def row(address, size, status, symbol):
    return address, size, status, "-", "-", symbol


class SmallestFirstTests(unittest.TestCase):
    def test_strict_original_size_and_address_priority(self):
        rows = [row(0x1080, 46, "UNRESOLVED", "__tcf_1"),
                row(0x1050, 31, "UNRESOLVED", "_GLOBAL__I_foo"),
                row(0x1070, 39, "UNRESOLVED", "bar"),
                row(0x1010, 31, "EXPERIMENTAL", "baz"),
                row(0x1030, 6, "EXACT", "foo"),
                row(0x1040, 0, "UNRESOLVED", "unknown")]
        result = small.make_queue(rows, {0x1070: "lexer.cpp"},
                                  {(0x1010, "baz"): ["vuc/config/baz_probe.json"]})
        self.assertEqual([x["address"] for x in result["pending"]],
                         ["0x00001010", "0x00001050", "0x00001070", "0x00001080"])
        self.assertEqual(result["pending"][1]["kind"], "startup:accepted-anchor")
        self.assertEqual(result["pending"][0]["probe_count"], 1)
        self.assertEqual(result["pending"][2]["original_local_source"], "lexer.cpp")
        self.assertEqual(result["tiers"], {32: 2, 64: 2})
        self.assertEqual(len(result["unsized"]), 1)

    def test_size_ceiling_does_not_promote_unresolved(self):
        rows = [row(0x1000, 31, "UNRESOLVED", "_GLOBAL__I_missing"),
                row(0x1020, 46, "UNRESOLVED", "__tcf_1"),
                row(0x1080, 85, "EXPERIMENTAL", "other")]
        result = small.make_queue(rows, {}, {}, max_size=64)
        self.assertEqual([x["size"] for x in result["pending"]], [31, 46])
        self.assertEqual(result["pending"][0]["kind"], "startup:unresolved-anchor")
        self.assertEqual(result["pending"][1]["kind"], "compiler:cleanup")
        self.assertTrue(all(x["status"] != "EXACT" for x in result["pending"]))

    def test_probes_are_evidence_hints_not_acceptance(self):
        with tempfile.TemporaryDirectory(dir=ROOT / "vuc/config") as scratch:
            p = Path(scratch) / "candidate_probe.json"
            p.write_text(json.dumps({"function": "func", "address": "0x1000"}))
            bad = Path(scratch) / "invalid_probe.json"
            bad.write_text("{oops")
            index = small.probe_index([p, bad])
            self.assertEqual(len(index[(0x1000, "func")]), 1)
            result = small.make_queue([row(0x1000, 31, "UNRESOLVED", "func")], {}, index)
            self.assertEqual(result["pending"][0]["status"], "UNRESOLVED")

    def test_real_original_elf_drives_queue(self):
        _, original = small.module("decomp").load()
        exact = json.loads((small.CONFIG / "exact_candidates.json").read_text())
        selected = json.loads((small.CONFIG / "selected_candidates.json").read_text())
        rows = small.module("coverage").classify(original, exact, selected)
        result = small.make_queue(rows, {}, {}, max_size=64)
        self.assertTrue(result["pending"])
        self.assertEqual(result["pending"][0]["size"], 46)
        self.assertTrue(all(row["size"] <= 64 for row in result["pending"]))
        self.assertTrue(all(row["status"] != "EXACT" for row in result["pending"]))


if __name__ == "__main__":
    unittest.main()

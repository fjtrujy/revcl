"""Validate original-address VUC reconstruction work-queue boundaries."""
import importlib.util
import tempfile
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_nonexact_work_queue", ROOT / "vuc/scripts/nonexact_work_queue.py")
queue = importlib.util.module_from_spec(spec)
spec.loader.exec_module(queue)


def entry(addr, size, status, symbol):
    return (addr, size, status, "-", "-", symbol)


class NonexactWorkQueueTests(unittest.TestCase):
    def test_exact_functions_split_nonexact_ranges(self):
        rows = [
            entry(0x1000, 12, "UNRESOLVED", "a"),
            entry(0x1010, 8, "EXPERIMENTAL", "b"),
            entry(0x1020, 4, "EXACT", "c"),
            entry(0x1040, 20, "UNRESOLVED", "d"),
            entry(0x1054, 0, "UNRESOLVED", "unknown_size"),
            entry(0x1060, 10, "EXACT", "e"),
        ]
        report = queue.build_queue(list(reversed(rows)), {0x1010: "local.cpp"}, top=10)
        self.assertEqual(report["summary"]["exact_named_function_records"], 2)
        self.assertEqual(report["summary"]["experimental_named_function_records"], 1)
        self.assertEqual(report["summary"]["unresolved_named_function_records"], 3)
        self.assertEqual(report["summary"]["unresolved_positive_size_functions"], 3)
        self.assertEqual(report["summary"]["nonexact_named_symbol_bytes"], 40)
        self.assertEqual(report["summary"]["unresolved_runs"], 2)
        self.assertEqual(report["largest_nonexact_runs"][0]["symbol_bytes"], 20)
        self.assertEqual(report["largest_nonexact_runs"][1]["symbol_bytes"], 20)
        self.assertEqual(report["smallest_nonexact"][0]["size"], 8)
        self.assertEqual(report["smallest_nonexact"][0]["source_file"], "local.cpp")
        self.assertIsNone(report["smallest_nonexact"][1]["source_file"])

    def test_original_address_duplicates_and_overlaps_rejected(self):
        duplicate = [entry(0x1000, 8, "UNRESOLVED", "one"),
                     entry(0x1000, 8, "EXACT", "two")]
        with self.assertRaisesRegex(ValueError, "duplicate original"):
            queue.build_queue(duplicate, {})
        overlap = [entry(0x1000, 16, "UNRESOLVED", "one"),
                   entry(0x1008, 8, "EXPERIMENTAL", "two")]
        with self.assertRaisesRegex(ValueError, "overlapping original"):
            queue.build_queue(overlap, {})

    def test_local_source_ownership_does_not_infer_global_symbols(self):
        with tempfile.TemporaryDirectory() as name:
            p = Path(name) / "sources.tsv"
            p.write_text(
                "file\tsymbol_index\tlocal_text_function_count\tlocal_text_addresses\n"
                "alpha.cpp\t10\t2\t0x00001000;0x00002000\n"
                "empty.cpp\t11\t0\t-\n"
            )
            self.assertEqual(queue.local_source_map(p),
                             {0x1000: "alpha.cpp", 0x2000: "alpha.cpp"})
            with p.open("a") as f:
                f.write("other.cpp\t12\t1\t0x00001000\n")
            with self.assertRaisesRegex(ValueError, "ambiguous"):
                queue.local_source_map(p)

    def test_startup_names_only_affect_shortlist_not_acceptance(self):
        self.assertTrue(queue.is_compiler_startup_name("__tcf_1"))
        self.assertTrue(queue.is_compiler_startup_name("_GLOBAL__I_matchStack"))
        self.assertTrue(queue.is_compiler_startup_name("__libc_csu_init"))
        self.assertTrue(queue.is_compiler_startup_name(
            "_Z41__static_initialization_and_destruction_0ii"))
        self.assertFalse(queue.is_compiler_startup_name("_ZN11SymbolTable4FindERK12MLGameStringb"))
        rows = [entry(0x1000, 16, "UNRESOLVED", "__tcf_1"),
                entry(0x1010, 17, "UNRESOLVED", "_Z9Applicationv")]
        report = queue.build_queue(rows, {}, top=5)
        self.assertEqual(report["summary"]["unresolved_named_function_records"], 2)
        self.assertEqual(len(report["smallest_nonexact"]), 2)
        self.assertEqual([f["symbol"] for f in report["smallest_nonexact_nonstartup"]],
                         ["_Z9Applicationv"])

    def test_canonical_rows_are_derived_from_authenticated_elf(self):
        _, info = queue.load_module("decomp").load()
        import json
        exact = json.loads((queue.CONFIG / "exact_candidates.json").read_text())
        selected = json.loads((queue.CONFIG / "selected_candidates.json").read_text())
        rows = queue.load_module("coverage").classify(info, exact, selected)
        report = queue.build_queue(rows, queue.local_source_map(
            queue.CONFIG / "source_units.tsv"))
        self.assertEqual(sum(report["summary"][f"{s.lower()}_named_function_records"]
                             for s in ("EXACT", "EXPERIMENTAL", "UNRESOLVED")),
                         len(info["functions"]))
        self.assertEqual(report["summary"]["exact_named_function_records"], len(exact))
        self.assertTrue(report["summary"]["unresolved_positive_size_functions"] > 0)
        self.assertIn("NOT a binary score", report["summary"]["note"])
        for section in report["largest_nonexact_runs"]:
            self.assertLess(int(section["start"], 0),
                            int(section["end_of_last_symbol"], 0))


if __name__ == "__main__":
    unittest.main()

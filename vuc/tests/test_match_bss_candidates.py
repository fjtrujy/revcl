"""Tests for original VUC uninitialized allocation declarations.

No NOBITS bytes are counted as exact executable file bytes.
"""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_bss_candidate_gate", ROOT/"vuc/scripts/match_bss_candidates.py")
bss = importlib.util.module_from_spec(spec)
spec.loader.exec_module(bss)


class BssCandidateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.records = json.loads(bss.CONFIG.read_text())
        cls.original, cls.target = bss.data.match.decomp.load()

    def test_original_twelve_nonzero_sized_unique_bss_symbols(self):
        self.assertEqual(len(self.records), 12)
        self.assertEqual(len({(r["symbol"], r["address"]) for r in self.records}), 12)
        self.assertEqual(sum(r["size"] for r in self.records), 18437)
        for r in self.records:
            self.assertTrue(r["source"].startswith("vuc/src/"))
            self.assertTrue(bss.within_vuc_src(r["source"]).is_file())
            symbol = bss.original_record(r, self.original, self.target)
            self.assertEqual(symbol[0], int(r["address"], 0))
            self.assertEqual(symbol[1], r["size"])

    def test_mutated_original_identity_is_rejected(self):
        row = self.records[0]
        with self.assertRaisesRegex(ValueError, "expected one authenticated original"):
            bss.original_record(dict(row, address="0x08080000"), self.original, self.target)
        with self.assertRaisesRegex(ValueError, "expected one authenticated original"):
            bss.original_record(dict(row, size=row["size"] + 1), self.original, self.target)
        with self.assertRaisesRegex(ValueError, "only original .bss"):
            bss.original_record(dict(row, section=".data"), self.original, self.target)

    def test_candidate_type_section_and_size_must_all_match(self):
        # SHT_NOBITS=8, SHF_ALLOC|SHF_WRITE=3.
        symbol = (0, 0, 2052, 0x11, 0, 1)
        sec = (0, 8, 3, 0, 0, 4096)
        record = {"symbol": "globalEnv", "size": 2052}
        sections = {".bss": (1, sec)}
        with patch.object(bss.data.match, "elf_sections", return_value=(sections, [None, sec])), \
             patch.object(bss.data, "read_named_symbols", return_value=[("globalEnv", symbol)]):
            self.assertEqual(bss.candidate_record(record, b"fake"), symbol)
        with patch.object(bss.data.match, "elf_sections", return_value=(sections, [None, sec])), \
             patch.object(bss.data, "read_named_symbols", return_value=[("globalEnv", symbol)]):
            with self.assertRaisesRegex(ValueError, "candidate symbol size"):
                bss.candidate_record(dict(record, size=2053), b"fake")
        for bad in [(0, 1, 3, 0, 0, 4096), (0, 8, 2, 0, 0, 4096),
                    (0, 8, 3, 0, 0, 1)]:
            sections = {".bss": (1, bad)}
            with patch.object(bss.data.match, "elf_sections", return_value=(sections, [None, bad])), \
                 patch.object(bss.data, "read_named_symbols", return_value=[("globalEnv", symbol)]):
                with self.assertRaises(ValueError):
                    bss.candidate_record(record, b"fake")

    def test_bss_gate_rejects_code_outside_reconstructed_sources(self):
        with self.assertRaisesRegex(ValueError, "source must exist"):
            bss.within_vuc_src("vuc/probes/scope_uninitialized_copy.cpp")
        with self.assertRaisesRegex(ValueError, "source must exist"):
            bss.within_vuc_src("../outside.cpp")

    def test_bss_gate_rejects_duplicate_before_compilation(self):
        # Duplicates should fail at the original-identity guard, rather than
        # using one object to artificially inflate verified declarations.
        original = self.records[0]
        with tempfile.TemporaryDirectory() as scratch:
            with patch.object(bss.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as compiler, \
                 patch.object(bss, "candidate_record") as candidate, \
                 patch.object(bss.Path, "read_bytes", return_value=b"fake"):
                with self.assertRaisesRegex(ValueError, "duplicate original BSS"):
                    bss.run("fake-compiler", scratch, [original, original])
                self.assertEqual(compiler.call_count, 1)
                self.assertEqual(candidate.call_count, 1)


if __name__ == "__main__":
    unittest.main()

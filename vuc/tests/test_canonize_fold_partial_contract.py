"""Original ELF acceptance boundaries for the Canonize_FoldExpr partial.

This is a host-side oracle for immutable target facts. It deliberately
does not assert the partial compiler object is byte exact; that requires
the historical GCC matching probe in the Linux guest.
"""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
SPEC = importlib.util.spec_from_file_location("fold_decomp", VUC / "scripts/decomp.py")
decomp = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(decomp)
CONFIG_PATH = VUC / "config/canonize_fold_expr_partial_oct16.json"


class CanonizeFoldPartialContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.target = decomp.load()
        cls.sections = {sec["name"]: sec for sec in cls.target["sections"]}
        cls.config = json.loads(CONFIG_PATH.read_text())

    def test_candidate_identifies_original_unresolved_function(self):
        config = self.config
        self.assertEqual(config["function"], "_Z17Canonize_FoldExprPN2IR9Expr_DataE")
        self.assertEqual(int(config["address"], 0), 0x080502c0)
        self.assertEqual(config["size"], 410)
        self.assertEqual(len([
            row for row in self.target["functions"]
            if row["address"] == 0x080502c0
            and row["name"] == config["function"] and row["size"] == 410
        ]), 1)

    def test_original_diagnostic_literal_is_byte_identical(self):
        span = self.config["relocation_literals"][".rodata"]
        self.assertEqual(len(span), 1)
        literal = span[0]
        self.assertEqual(literal, {
            "offset": 0, "size": 31, "address": "0x08072460"})
        section = self.sections[".rodata"]
        start = section["offset"] + int(literal["address"], 0) - section["address"]
        self.assertEqual(self.raw[start:start + 31], b"illegal operand in const expr!\0")

    def test_partial_is_not_in_exact_acceptance(self):
        name = "vuc/config/canonize_fold_expr_partial_oct16.json"
        selected = json.loads((VUC / "config/selected_candidates.json").read_text())
        exact = json.loads((VUC / "config/exact_candidates.json").read_text())
        self.assertIn(name, selected)
        self.assertNotIn(name, exact)
        self.assertTrue(self.config["source"].startswith("vuc/probes/"))
        self.assertEqual(len(self.config["relocations"]), 12)


if __name__ == "__main__":
    unittest.main()

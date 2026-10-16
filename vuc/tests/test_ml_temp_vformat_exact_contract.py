"""Original-ELF source and signed-branch contract for VFormatString exact match."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
spec = importlib.util.spec_from_file_location("vformat_decomp", VUC / "scripts/decomp.py")
decomp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decomp)
CONFIG = "vuc/config/ml_temp_vformat_assert_exact_oct16.json"


class MLTempVFormatExactContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.target = decomp.load()
        cls.sections = {s["name"]: s for s in cls.target["sections"]}
        cls.config = json.loads((ROOT / CONFIG).read_text())

    @classmethod
    def original_bytes(cls, section_name, address, size):
        section = cls.sections[section_name]
        off = address - section["address"]
        assert 0 <= off <= section["size"] - size
        return cls.raw[section["offset"] + off:section["offset"] + off + size]

    def test_exact_function_is_accepted_and_historical_flags_are_pinned(self):
        selected = json.loads((VUC / "config/selected_candidates.json").read_text())
        exact = json.loads((VUC / "config/exact_candidates.json").read_text())
        self.assertIn(CONFIG, selected)
        self.assertIn(CONFIG, exact)
        self.assertEqual(self.config["function"],
                         "_ZN16MLTempStringData13VFormatStringEPKcPc")
        self.assertEqual(self.config["address"], "0x08071a50")
        self.assertEqual(self.config["size"], 238)
        self.assertEqual(self.config["source"], "vuc/src/MLTempVFormatExact.cpp")
        self.assertEqual(self.config["flags"],
                         ["-O2", "-frename-registers", "-falign-jumps=1"])
        self.assertEqual(len(self.config["relocations"]), 4)

    def test_original_signed_conditions_match_int_max_contract(self):
        # jge rel8 at original +0x31 and jle rel32 at +0x3d.
        self.assertEqual(self.original_bytes(".text", 0x08071a50 + 0x31, 1),
                         b"\x7d")
        self.assertEqual(self.original_bytes(".text", 0x08071a50 + 0x3d, 2),
                         b"\x0f\x8e")
        source = (ROOT / self.config["source"]).read_text()
        self.assertIn("int Max() const { return 0x1fbf; }", source)

    def test_original_five_literal_cluster_is_contiguous(self):
        # The compiler emits 160 bytes including the original seven
        # alignment zeros. This is whole-section ownership, not
        # independent arbitrary string relocation targets.
        address = int(self.config["relocations"][".rodata"], 0)
        cluster = self.original_bytes(".rodata", address, 160)
        self.assertEqual(cluster[:6], b"char* ")
        self.assertEqual(cluster[0x39:0x40], b"\0" * 7)
        self.assertIn(b"char* MLTempStringData::Reserve(int)\0", cluster)
        self.assertIn(b"numWritten >= 0\0", cluster)
        self.assertIn(b"../megalib/MLGameString.cpp\0", cluster)
        self.assertTrue(cluster.endswith(b"Max() > strLen\0"))


if __name__ == "__main__":
    unittest.main()

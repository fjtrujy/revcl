"""Verify canonical ELF-derived per-translation-unit iostream cleanup records."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_iostream_cleanup", ROOT / "vuc/scripts/iostream_cleanup_inventory.py")
inventory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inventory)


class IostreamCleanupInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.target = inventory.match.decomp.load()
        cls.records = inventory.derive(cls.raw, cls.target)
        cls.initializers = inventory.derive_initializers(cls.raw, cls.target)

    def test_original_elf_has_twenty_distinct_thunks(self):
        self.assertEqual(len(self.records), 20)
        self.assertEqual(len({x["address"] for x in self.records}), 20)
        self.assertTrue(all(x["function"] == "__tcf_0" and x["size"] == 16
                            for x in self.records))
        self.assertEqual(len({x["relocation_objects"][".bss"]["_ZSt8__ioinit"]
                              for x in self.records}), 20)

    def test_derived_configurations_match_checked_in_original_addresses(self):
        for rec in self.records:
            self.assertEqual(json.loads(inventory.path_for(rec).read_text()), rec)
            self.assertEqual(rec["relocations"]["_ZNSt8ios_base4InitD1Ev"],
                             "0x080499dc")

    def test_unexpected_original_cleanup_bytes_are_rejected(self):
        modified = bytearray(self.raw)
        section = next(x for x in self.target["sections"] if x["name"] == ".text")
        addr = int(self.records[0]["address"], 0)
        loc = section["offset"] + addr - section["address"]
        modified[loc] ^= 1
        with self.assertRaisesRegex(ValueError, "unrecognized original"):
            inventory.derive(bytes(modified), self.target)

    def test_thirteen_other_66_byte_initializers_are_original_elf_derived(self):
        self.assertEqual(len(self.initializers), 13)
        self.assertEqual(len({x["address"] for x in self.initializers}), 13)
        self.assertTrue(all(x["size"] == 66 and x["source"] == inventory.SOURCE
                            for x in self.initializers))
        self.assertNotIn("0x08049af0", {x["address"] for x in self.initializers})

    def test_initializer_configs_are_deterministic_and_have_real_callbacks(self):
        callbacks = {int(x["address"], 0) for x in self.records}
        for item in self.initializers:
            self.assertEqual(json.loads(inventory.initializer_path_for(item).read_text()), item)
            ref = int(item["relocations"][".text.__tcf_0"], 0)
            self.assertIn(ref, callbacks)
            self.assertEqual(item["relocations"]["_ZNSt8ios_base4InitC1Ev"],
                             "0x0804974c")

    def test_corrupted_original_initializer_rejected(self):
        modified = bytearray(self.raw)
        section = next(x for x in self.target["sections"] if x["name"] == ".text")
        addr = int(self.initializers[0]["address"], 0)
        loc = section["offset"] + addr - section["address"]
        modified[loc] ^= 1
        with self.assertRaisesRegex(ValueError, "unrecognized original"):
            inventory.derive_initializers(bytes(modified), self.target)


if __name__ == "__main__":
    unittest.main()

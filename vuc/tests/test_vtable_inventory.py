"""Original ELF-derived polymorphic vtable inventory and emitted-source tests."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vtable_inventory", ROOT / "vuc/scripts/vtable_inventory.py")
inventory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inventory)


class OriginalVtableInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original_raw, cls.target = inventory.inv.decomp.load()
        cls.names, cls.info, cls.vtables = inventory.inventory(
            cls.original_raw, cls.target)

    def test_original_12_byte_vtables_use_authenticated_inherited_methods(self):
        self.assertEqual(len(self.vtables), 67)
        self.assertEqual(sum(x["group"] == "IR" for x in self.vtables), 44)
        self.assertEqual(sum(x["group"] == "top" for x in self.vtables), 23)
        self.assertEqual(len({x["symbol"] for x in self.vtables}), 67)
        for item in self.vtables:
            self.assertEqual(item["size"], 12)
            self.assertEqual(set(item["relocations"]), {"0x04", "0x08"})
            self.assertEqual(item["relocations"]["0x04"]["type"], 1)
            self.assertEqual(item["relocations"]["0x08"]["type"], 2)
            self.assertIn(item["relocations"]["0x08"]["symbol"],
                          {x[0] for x in inventory.BASE_METHODS.values()})

    def test_vtable_manifests_and_source_are_reproducible(self):
        manifest = json.loads((ROOT/"vuc/config/exact_data_candidates.json").read_text())
        for item in self.vtables:
            path = inventory.config_path(item)
            checked = {k: v for k, v in item.items() if k not in ("group", "type")}
            self.assertEqual(json.loads(path.read_text()), checked)
            self.assertIn(str(path.relative_to(ROOT)), manifest)
        for group, source in inventory.SOURCE.items():
            expected = inventory.vtable_source(
                group, self.names, self.info, self.vtables)
            actual = (ROOT/source).read_text()
            self.assertEqual(actual, expected)
            self.assertIn("NOT original VUC global objects", actual)
            self.assertNotIn(".long", actual)
            self.assertNotIn(".byte", actual)

    def test_all_original_named_class_vtables_have_exact_manifests(self):
        original = {r["name"] for r in self.target["symbols"]
                    if r["type"] == 1 and r["section"] == ".rodata"
                    and r["name"].startswith("_ZTV")}
        self.assertEqual(len(original), 69)
        manifest = json.loads((ROOT/"vuc/config/exact_data_candidates.json").read_text())
        checked = {json.loads((ROOT/p).read_text())["symbol"]
                   for p in manifest}
        self.assertEqual(original, original.intersection(checked))
        self.assertEqual(sum(x["symbol"].startswith("_ZTV")
                             for x in (json.loads((ROOT/p).read_text()) for p in manifest)), 69)

    def test_original_corrupt_virtual_function_pointer_is_rejected(self):
        victim = self.vtables[0]
        section = next(x for x in self.target["sections"] if x["name"] == ".rodata")
        position = section["offset"] + int(victim["address"], 0) - section["address"] + 8
        modified = bytearray(self.original_raw)
        modified[position] ^= 1
        with self.assertRaisesRegex(ValueError, "unrecognized original virtual slot"):
            inventory.inventory(bytes(modified), self.target)

    def test_original_corrupt_typeinfo_pointer_is_rejected(self):
        victim = self.vtables[0]
        section = next(x for x in self.target["sections"] if x["name"] == ".rodata")
        position = section["offset"] + int(victim["address"], 0) - section["address"] + 4
        modified = bytearray(self.original_raw)
        modified[position] ^= 1
        with self.assertRaisesRegex(ValueError, "unexpected original vtable header"):
            inventory.inventory(bytes(modified), self.target)

    def test_source_metadata_cannot_be_decoupled_from_original_bytes(self):
        with self.assertRaisesRegex(ValueError, "must be supplied together"):
            inventory.inventory(self.original_raw, None)


if __name__ == "__main__":
    unittest.main()

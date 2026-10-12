"""Independent ELF-derived C++ RTTI type-name source inventory checks."""
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_rtti_names", ROOT/"vuc/scripts/rtti_name_inventory.py")
inventory = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inventory)


class OriginalRttiNameTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.raw, cls.original = inventory.decomp.load()
        cls.records = inventory.original_type_names(cls.raw, cls.original)
        cls.typeinfos = inventory.original_type_infos(cls.raw, cls.original, cls.records)

    def test_all_original_type_names_are_derived_from_real_elf(self):
        self.assertEqual(len(self.records), 81)
        self.assertEqual(sum(r["size"] for r in self.records), 1324)
        self.assertEqual(sum(r["group"] == "IR" for r in self.records), 51)
        self.assertEqual(sum(r["group"] == "top" for r in self.records), 30)
        self.assertEqual(len({r["symbol"] for r in self.records}), 81)

    def test_manifest_matches_exact_elf_generated_records(self):
        names = json.loads((ROOT/"vuc/config/exact_data_candidates.json").read_text())
        self.assertEqual(len(names), 240)
        for record in self.records:
            path = inventory.config_path(record)
            self.assertTrue(path.is_file())
            self.assertEqual(json.loads(path.read_text()), inventory.data_config(record))
            self.assertIn(str(path.relative_to(ROOT)), names)
        for record in self.typeinfos:
            path = inventory.typeinfo_config_path(record)
            self.assertEqual(json.loads(path.read_text()),
                             inventory.typeinfo_config(record, self.records, self.typeinfos))
            self.assertIn(str(path.relative_to(ROOT)), names)

    def test_sources_are_deterministic_inheritance_only_declarations(self):
        for group, path in inventory.SOURCES.items():
            expected = inventory.cxx_source(group, self.records, self.typeinfos)
            actual = (ROOT/path).read_text()
            self.assertEqual(actual, expected)
            self.assertNotRegex(actual, r"(?m)^\s*virtual\s")
            self.assertNotIn("operator new", actual)
            self.assertEqual(actual.count("typeid("),
                             sum(r["group"] == group for r in self.records))

    def test_original_rtti_graph_has_nine_roots_and_seventy_two_single_bases(self):
        self.assertEqual(len(self.typeinfos), 81)
        self.assertEqual(sum(r["size"] for r in self.typeinfos), 936)
        self.assertEqual(sum(r["parent_symbol"] is None for r in self.typeinfos), 9)
        self.assertEqual(sum(r["parent_symbol"] is not None for r in self.typeinfos), 72)
        names = {r["symbol"] for r in self.typeinfos}
        self.assertTrue(all(r["parent_symbol"] in names
                            for r in self.typeinfos if r["parent_symbol"]))

    def test_corrupted_original_inheritance_pointer_is_rejected(self):
        modified = bytearray(self.raw)
        derived = next(r for r in self.typeinfos if r["parent_symbol"])
        section = next(s for s in self.original["sections"] if s["name"] == ".rodata")
        pointer_offset = section["offset"] + int(derived["address"], 0) - section["address"] + 8
        modified[pointer_offset] ^= 1
        with self.assertRaisesRegex(ValueError, "unrecognized original RTTI parent"):
            inventory.original_type_infos(bytes(modified), self.original, self.records)

    def test_cyclic_source_inheritance_is_rejected(self):
        records = [dict(r) for r in self.typeinfos]
        row = next(r for r in records if r["parent_symbol"])
        row["parent_symbol"] = row["symbol"]
        with self.assertRaisesRegex(ValueError, "cycle in authenticated"):
            inventory.cxx_source(row["group"], self.records, records)

    def test_original_type_name_string_mismatch_is_rejected(self):
        modified = bytearray(self.raw)
        entry = self.records[0]
        section = next(s for s in self.original["sections"] if s["name"] == ".rodata")
        addr = int(entry["address"], 0)
        offset = section["offset"] + addr - section["address"]
        modified[offset] ^= 1
        with self.assertRaisesRegex(ValueError, "not canonical"):
            inventory.original_type_names(bytes(modified), self.original)


if __name__ == "__main__":
    unittest.main()

"""Tests for VUC's independent, strict named initialized-data acceptance gate."""
import importlib.util
import json
from pathlib import Path
import subprocess
import struct
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_match_data_candidates", ROOT/"vuc/scripts/match_data_candidates.py")
suite = importlib.util.module_from_spec(spec)
spec.loader.exec_module(suite)


class NamedDataCandidateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.configs = json.loads(suite.EXACT_MANIFEST.read_text())
        cls.canonical_raw, cls.canonical_target = suite.matcher.match.decomp.load()

    def test_original_initialized_data_and_rtti_objects(self):
        self.assertEqual(len(self.configs), 244)
        records = [json.loads(suite.within_repo(name).read_text()) for name in self.configs]
        self.assertEqual(len({(r["symbol"], r["address"]) for r in records}), 244)
        self.assertEqual(sum(r["size"] for r in records), 71536)
        for r in records:
            if r["symbol"].startswith(("_ZTS", "_ZTI", "_ZTV")):
                self.assertEqual(r["section"], ".rodata")
                if r["symbol"].startswith("_ZTV"):
                    if r["symbol"] in ("_ZTV11yyFlexLexer", "_ZTV9FlexLexer"):
                        self.assertEqual(r["source"], "vuc/src/FlexLexerDestructors.cpp")
                        self.assertEqual(len(r["relocations"]),
                                         12 if r["symbol"] == "_ZTV11yyFlexLexer" else 9)
                    else:
                        self.assertIn(r["source"], ("vuc/src/IRVtableProbes.cpp",
                                                     "vuc/src/ASTVtableProbes.cpp"))
                        self.assertEqual(len(r["relocations"]), 2)
                else:
                    self.assertIn(r["source"], ("vuc/src/IRRttiNames.cpp",
                                               "vuc/src/ASTRttiNames.cpp"))
                if r["symbol"].startswith("_ZTS"):
                    self.assertEqual(r["relocations"], {})
                elif r["symbol"].startswith("_ZTI"):
                    self.assertEqual(len(r["relocations"]), 2 if r["size"] == 8 else 3)
            elif (r["symbol"] in ("_fp_hw", "_IO_stdin_used")
                  or r["symbol"].endswith("_S_pback_sizeE")):
                self.assertEqual(r["section"], ".rodata")
                self.assertEqual(r["relocations"], {})
            else:
                self.assertEqual(r["section"], ".data")
            if r["source"] == "vuc/src/GlobalBuffers.cpp":
                self.assertEqual(r["relocations"], {})
                self.assertIn("-fno-zero-initialized-in-bss", r["flags"])
            addr, length, section_id, name = suite.matcher.original_symbol(
                self.canonical_raw, self.canonical_target,
                r["symbol"], r["address"], r["size"], 1)
            self.assertEqual(addr, int(r["address"], 0))
            self.assertEqual(length, r["size"])
            self.assertEqual(name, r["section"])
            sec = self.canonical_target["sections"][section_id]
            begin = sec["offset"] + addr - sec["address"]
            if r["symbol"].startswith(("_ZTI", "_ZTV")):
                reconstructed = bytearray(length)
                for offset, target in r["relocations"].items():
                    pointer = int(target["address"], 0)
                    if target["symbol"].startswith("_ZTVN10__cxxabiv1"):
                        pointer += 8  # Original C++ ABI typeinfo vtable address-point
                    struct.pack_into("<I", reconstructed, int(offset, 0), pointer)
                expected = bytes(reconstructed)
            elif r["symbol"] in ("_fp_hw", "_IO_stdin_used"):
                expected = struct.pack("<I", 3 if r["symbol"] == "_fp_hw" else 0x20001)
            elif r["symbol"].endswith("_S_pback_sizeE"):
                expected = struct.pack("<I", 1)
            else:
                expected = ((r["symbol"][4:] + "\0").encode("ascii")
                            if r["symbol"].startswith("_ZTS")
                            else b"\x86\x22\x07\x08" if r["symbol"] == "currFileName"
                            else b"\x01\x00\x00\x00" if r["symbol"].endswith("E2id")
                            else bytes(length))
            self.assertEqual(self.canonical_raw[begin:begin + length], expected,
                             r["symbol"])

    def test_all_244_are_built_in_eleven_translation_units_and_must_be_exact(self):
        def matched(obj, config):
            record = json.loads(Path(config).read_text())
            return {"symbol": record["symbol"], "size": record["size"],
                    "exact_bytes": record["size"], "exact": True,
                "resolved_relocations": len(record["relocations"])}
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")) as compiler:
                with patch.object(suite.matcher, "run", side_effect=matched):
                    result = suite.run("compiler", folder, self.configs)
            self.assertEqual(compiler.call_count, 11)
            self.assertEqual(result["data_objects_checked"], 244)
            self.assertEqual(result["fully_exact_data_objects"], 244)
            self.assertEqual(result["checked_data_bytes"], 71536)
            self.assertEqual(result["matching_data_bytes"], 71536)
            self.assertEqual(result["resolved_relocations"], 390)
            self.assertTrue(result["all_candidates_exact"])
            self.assertIsNone(result["whole_binary_score"])
            self.assertFalse(result["whole_binary_byte_exact"])
            self.assertTrue((Path(folder)/"data_candidate_summary.json").is_file())

    def test_partial_match_rejects_whole_set(self):
        def fail(obj, config):
            rec = json.loads(Path(config).read_text())
            return {"symbol": rec["symbol"], "size": rec["size"],
                    "exact_bytes": rec["size"]-1, "exact": False,
                    "resolved_relocations": 0}
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run", side_effect=fail):
                    result = suite.run("compiler", folder, self.configs)
            self.assertEqual(result["acceptance"], "FAIL")
            self.assertEqual(result["fully_exact_data_objects"], 0)
            self.assertEqual(result["matching_data_bytes"], 71536-244)

    def test_pointer_literal_relocation_requires_both_original_and_candidate_literal(self):
        record = json.loads(
            suite.within_repo("vuc/config/data_currFileName.json").read_text())
        relocation = record["relocations"]["0x0"]
        matcher = suite.matcher
        headers = [(0, 0, 0, 0, 0, 0), (0, 1, 0, 0, 0, 0)]
        section_symbol = (0, 0, 0, 3, 0, 1)
        with patch.object(matcher.match, "section_bytes", return_value=b"\x00"):
            self.assertEqual(matcher.verify_literal_relocation(
                b"", headers, section_symbol, self.canonical_raw,
                self.canonical_target, relocation, 0), 0x08072286)
            with self.assertRaisesRegex(ValueError, "addend"):
                matcher.verify_literal_relocation(
                    b"", headers, section_symbol, self.canonical_raw,
                    self.canonical_target, relocation, -1)
            with self.assertRaisesRegex(ValueError, "original ELF literal"):
                modified = bytearray(self.canonical_raw)
                section = next(x for x in self.canonical_target["sections"]
                               if x["name"] == ".rodata")
                offset = section["offset"] + 0x08072286 - section["address"]
                modified[offset] = ord("X")
                matcher.verify_literal_relocation(
                    b"", headers, section_symbol, bytes(modified),
                    self.canonical_target, relocation, 0)
        with patch.object(matcher.match, "section_bytes", return_value=b"X"):
            with self.assertRaisesRegex(ValueError, "candidate literal"):
                matcher.verify_literal_relocation(
                    b"", headers, section_symbol, self.canonical_raw,
                    self.canonical_target, relocation, 0)

    def test_literal_relocation_accepts_checked_nonzero_candidate_section_offset(self):
        matcher = suite.matcher
        headers = [(0, 0, 0, 0, 0, 0), (0, 1, 0, 0, 0, 0)]
        section_symbol = (0, 0, 0, 3, 0, 1)
        record = {
            "symbol": ".rodata", "type": 3,
            "original_section": ".rodata", "address": "0x08072316",
            "literal_hex": b"int\x00".hex(),
        }
        # The candidate string's section-relative offset is seven, but its
        # authenticated original address is *already* the linked destination.
        # It must not be added a second time during ELF relocation resolution.
        with patch.object(matcher.match, "section_bytes",
                          return_value=b"unused\x00int\x00"):
            self.assertEqual(matcher.verify_literal_relocation(
                b"", headers, section_symbol, self.canonical_raw,
                self.canonical_target, record, 7), 0x08072316)
            with self.assertRaisesRegex(ValueError, "candidate literal"):
                matcher.verify_literal_relocation(
                    b"", headers, section_symbol, self.canonical_raw,
                    self.canonical_target, record, 6)
            with self.assertRaisesRegex(ValueError, "addend"):
                matcher.verify_literal_relocation(
                    b"", headers, section_symbol, self.canonical_raw,
                    self.canonical_target, record, -1)
            with self.assertRaisesRegex(ValueError, "original ELF literal"):
                wrong = dict(record, address="0x0807231a")
                matcher.verify_literal_relocation(
                    b"", headers, section_symbol, self.canonical_raw,
                    self.canonical_target, wrong, 7)

    def test_duplicate_and_escape_are_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            with patch.object(suite.subprocess, "run",
                              return_value=subprocess.CompletedProcess([], 0, "", "")):
                with patch.object(suite.matcher, "run",
                                  return_value={"exact": True, "size": 1024,
                                                "exact_bytes": 1024,
                                                "resolved_relocations": 0}):
                    with self.assertRaisesRegex(ValueError, "duplicate original"):
                        suite.run("compiler", folder, [self.configs[0]] * 2)
            with self.assertRaisesRegex(ValueError, "escapes repository"):
                suite.run("compiler", folder, ["../../etc/passwd"])
            with self.assertRaisesRegex(ValueError, "nonempty"):
                suite.run("compiler", folder, [])


if __name__ == "__main__":
    unittest.main()

"""Full-ELF byte-identity checks must not accept selected-function matches."""
import importlib.util
import contextlib
import io
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "vuc_whole_elf_gate", ROOT / "vuc/scripts/whole_elf_gate.py")
# Whole gate uses the same local decomp.py as the other VUC scripts.
import sys
sys.path.insert(0, str(ROOT / "vuc/scripts"))
gate = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gate)


class WholeELFGateTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.original, cls.info = gate.decomp.load()

    def test_identical_original_bytes_pass(self):
        result = gate.compare(self.original, bytes(self.original))
        self.assertEqual(result["acceptance"], "PASS")
        self.assertTrue(result["whole_elf_byte_exact"])
        self.assertEqual(result["original_sha256"], gate.decomp.CANONICAL_SHA256)
        self.assertIsNone(result["first_different_file_offset"])

    def test_changed_text_byte_fails_even_if_file_size_is_identical(self):
        text = next(s for s in self.info["sections"] if s["name"] == ".text")
        changed = bytearray(self.original)
        offset = text["offset"] + 32
        changed[offset] ^= 1
        result = gate.compare(self.original, bytes(changed))
        self.assertEqual(result["acceptance"], "FAIL")
        self.assertFalse(result["whole_elf_byte_exact"])
        self.assertEqual(result["first_different_file_offset"], offset)
        self.assertNotEqual(result["candidate_sha256"], result["original_sha256"])

    def test_changed_data_byte_fails(self):
        rodata = next(s for s in self.info["sections"] if s["name"] == ".rodata")
        changed = bytearray(self.original)
        changed[rodata["offset"]] ^= 1
        result = gate.compare(self.original, bytes(changed))
        self.assertEqual(result["acceptance"], "FAIL")
        self.assertEqual(result["first_different_file_offset"], rodata["offset"])

    def test_appended_and_truncated_files_fail(self):
        for changed in (self.original + b"\0", self.original[:-1]):
            result = gate.compare(self.original, changed)
            self.assertEqual(result["acceptance"], "FAIL")
            self.assertEqual(result["first_different_file_offset"],
                             min(len(self.original), len(changed)))

    def test_cli_exits_nonzero_for_different_elf(self):
        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / "rebuilt.elf"
            candidate.write_bytes(self.original)
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(gate.main(["--candidate", str(candidate)]), 0)
            altered = bytearray(self.original)
            altered[-1] ^= 1
            candidate.write_bytes(altered)
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(gate.main(["--candidate", str(candidate)]), 1)
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(gate.main(["--candidate", str(candidate) + ".missing"]), 2)


if __name__ == "__main__":
    unittest.main()

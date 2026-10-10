"""Compiler-probe manifest and failure-mode tests."""
import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "vuc/scripts/compiler_probe.py"
spec = importlib.util.spec_from_file_location("vuc_probe", SCRIPT)
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


class CompilerProbeTests(unittest.TestCase):
    def test_missing_compiler_fails(self):
        with tempfile.TemporaryDirectory() as out:
            with self.assertRaises(OSError):
                probe.run_probe("/not-a-real-compiler", out, {"O0": ["-O0"]})

    @unittest.skipUnless(shutil.which("clang++"), "host compiler unavailable")
    def test_host_smoke_does_not_claim_historical_match(self):
        with tempfile.TemporaryDirectory() as out:
            report = probe.run_probe(shutil.which("clang++"), out, {"O0": ["-O0"]})
            self.assertFalse(report["historical_toolchain_verified"])
            self.assertIn("sha256", report["candidates"]["O0"])
            self.assertTrue((Path(out) / "manifest.json").is_file())


if __name__ == "__main__":
    unittest.main()

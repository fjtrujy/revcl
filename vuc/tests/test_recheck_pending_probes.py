"""The pending probe scanner never double-counts accepted symbol identities."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    "vuc_pending", ROOT / "vuc/scripts/recheck_pending_probes.py")
pending = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pending)


class RecheckPendingTests(unittest.TestCase):
    def test_accepted_alias_is_not_an_unaccepted_discovery(self):
        # The exact manifest already names this original function through a
        # separate config, so no candidate build should be necessary.
        alt = ROOT / "vuc/config/list_dispatch_bind_var_probe.json"
        with tempfile.TemporaryDirectory() as dirname:
            with patch.object(pending.subprocess, "run") as compiler:
                result = pending.scan("/unused/compiler", Path(dirname), [alt])
        self.assertEqual(result, [])
        compiler.assert_not_called()

    def test_original_elf_named_lifecycle_alias_not_double_counted(self):
        alt = ROOT / "vuc/config/ml_string_table_cleanup_probe.json"
        with tempfile.TemporaryDirectory() as dirname:
            with patch.object(pending.subprocess, "run") as compiler:
                result = pending.scan("/unused/compiler", Path(dirname), [alt])
        self.assertEqual(result, [])
        compiler.assert_not_called()


if __name__ == "__main__":
    unittest.main()

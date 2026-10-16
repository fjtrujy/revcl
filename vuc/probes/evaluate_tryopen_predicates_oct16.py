#!/usr/bin/env python3
"""TryOpenFile original stream-state predicate alternatives in GCC 3.3.3."""
from pathlib import Path
import importlib.util
import subprocess

ROOT = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-tryopen-predicates-oct16")
OUT.mkdir(exist_ok=True)
forms = {
    "baseline": "if (!f.fail()) return true; return false;",
    "good": "if (f.good()) return true; return false;",
    "bool": "if (f) return true; return false;",
    "not-bool": "if (!(!f)) return true; return false;",
    "is-open": "if (f.is_open()) return true; return false;",
    "not-bad-fail": "if (!f.bad() && !f.fail()) return true; return false;",
    "state-zero": "if (f.rdstate() == 0) return true; return false;",
    "failbit": "if (!(f.rdstate() & std::ios::failbit)) return true; return false;",
}
spec = importlib.util.spec_from_file_location("match", ROOT / "scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
best = (650, "baseline")
for name, body in forms.items():
    path = OUT / (name + ".cpp")
    path.write_text("#include <fstream>\nbool TryOpenFile(const char *path) {\n"
                    "  std::ifstream f(path);\n  " + body + "\n}\n")
    obj = OUT / (name + ".o")
    run = subprocess.run([
        "/tmp/vuc-binutils215/g++-jan25-as215", "-O2",
        "-frename-registers", "-falign-jumps=8", "-c",
        str(path), "-o", str(obj)], capture_output=True, text=True)
    if run.returncode:
        print("COMPILE_FAIL", name, run.stderr[-160:], flush=True)
        continue
    try:
        result = match.run(obj, ROOT / "config/try_open_file_partial_oct15t.json")
    except Exception as exc:
        print("REJECT", name, str(exc)[:180], flush=True)
        continue
    score = result["exact_bytes"]
    print("RESULT", name, result["length"], score,
          result["different_offsets"][:30], flush=True)
    if result["length"] == 754 and score > best[0]:
        best = score, name
        (OUT / "BEST").write_text(name + " " + str(score) + "\n")
    if result["exact"]:
        print("BYTE_EXACT", name, flush=True)
        (OUT / "EXACT").write_text(name + "\n")
print("BEST", best, flush=True)

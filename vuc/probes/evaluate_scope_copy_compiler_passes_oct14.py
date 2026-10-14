#!/usr/bin/env python3
"""Bounded GCC 3.3 compiler-pass controls for VUC's Scope copy cleanup.

No source rewriting or manually produced instruction bytes: this compares an
authentic compiler-generated function with the archived VUC instruction bytes.
"""
import importlib.util
import itertools
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("matcher", root / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
config = json.loads((root / "vuc/config/scope_copy_prefix_diagnostic_oct14_probe.json").read_text())
raw, target = matcher.decomp.load()
address = int(config["address"], 0)
text = next(s for s in target["sections"] if s["name"] == ".text")
start = text["offset"] + address - text["address"]
original = raw[start:start + config["size"]]
out = Path("/tmp/vuc-binutils215/scope-compiler-passes-oct14")
out.mkdir(parents=True, exist_ok=True)

passes = (
    ("normal", []),
    ("no-cse-follow-jumps", ["-fno-cse-follow-jumps"]),
    ("no-gcse", ["-fno-gcse"]),
    ("no-if-conversion", ["-fno-if-conversion"]),
    ("no-if-conversion2", ["-fno-if-conversion2"]),
    ("no-crossjumping", ["-fno-crossjumping"]),
    ("no-sched2", ["-fno-schedule-insns2"]),
    ("no-guess-prob", ["-fno-guess-branch-probability"]),
    ("no-reorder", ["-fno-reorder-blocks"]),
    ("no-merge-constants", ["-fno-merge-constants"]),
)
results = []
for snapshot, (pass_name, pass_flags), align in itertools.product(
    ("jan25", "dec06"), passes, (1, 4, 8)
):
    label = f"{snapshot}-{pass_name}-j{align}"
    obj = out / f"{label}.o"
    flags = ["-O2", "-frename-registers", f"-falign-jumps={align}", *pass_flags]
    command = [
        f"/tmp/vuc-binutils215/g++-{snapshot}-as215", *flags,
        "-c", str(root / config["source"]), "-o", str(obj)
    ]
    proc = subprocess.run(command, capture_output=True, text=True)
    if proc.returncode:
        results.append({"variant": label, "error": proc.stderr[:160]})
        continue
    try:
        generated, reloc = matcher.prepare_named_function(
            obj.read_bytes(), config["function"], config["relocations"], address
        )
    except ValueError as exc:
        results.append({"variant": label, "error": str(exc)})
        continue
    prefix = next((i for i, pair in enumerate(zip(generated, original))
                   if pair[0] != pair[1]), min(len(generated), len(original)))
    matching = sum(x == y for x, y in zip(generated, original))
    results.append({"variant": label, "length": len(generated),
                    "relocations": reloc, "prefix": prefix, "overlap": matching,
                    "full_byte_match": len(generated) == len(original) and
                    generated == original})

(out / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(json.dumps({
    "attempted": len(results),
    "original_length": len(original),
    "original_length_candidates": [r for r in results if r.get("length") == len(original)],
    "best_prefixes": sorted((r for r in results if "prefix" in r),
                            key=lambda r: (r["prefix"], r["overlap"]), reverse=True)[:8],
    "results": str(out / "results.json"),
}, indent=2))

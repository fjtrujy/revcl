#!/usr/bin/env python3
"""VUC Value addition scalar-label frequency: real case-local guard controls.

The 216-byte naturally ascending vector source matches useful arithmetic
instructions and branch topology but omits the 3-byte natural alignment
before the final case-3 scalar path. Test source-level redundant case guards
that an optimizer may eliminate while retaining legitimate case-local
control-flow probability evidence. Strictly diagnostic; no accepted source.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-value-case-guard-20261120")
OUT.mkdir(exist_ok=True)
src = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
old = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
new = """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""
assert src.count(old) == 1
src = src.replace(old, new, 1)
scalar = "case 3: result.values[0] += temporary.values[0]; break;"
assert src.count(scalar) == 1
scalar_action = "result.values[0] += temporary.values[0];"
vector_start = "case 4:\n" + new
assert src.count(vector_start) == 1
forms = {"natural": src}
for tag, guard in (
    ("scalar_plain", "result.type == 3"),
    ("scalar_likely", "__builtin_expect(result.type == 3, 1)"),
    ("scalar_unlikely", "__builtin_expect(result.type == 3, 0)"),
    ("scalar_nonzero", "__builtin_expect(result.type != 0, 1)"),
    ("scalar_not_four", "__builtin_expect(result.type != 4, 1)"),
    ("scalar_not_four_bad", "__builtin_expect(result.type != 4, 0)"),
):
    forms[tag] = src.replace(
        scalar, "case 3: if (" + guard + ") " + scalar_action + " break;", 1,
    )
for tag, guard in (
    ("vector_plain", "result.type == 4"),
    ("vector_likely", "__builtin_expect(result.type == 4, 1)"),
    ("vector_unlikely", "__builtin_expect(result.type == 4, 0)"),
):
    forms[tag] = src.replace(
        vector_start, "case 4:\n            if (" + guard + ") {\n"
        + new + "\n            }",
        1,
    )
spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
cfg = json.loads((VUC / "config/value_addition_partial_oct14_probe.json").read_text())
raw, elf = match.decomp.load()
section = next(s for s in elf["sections"] if s["name"] == ".text")
addr = int(cfg["address"], 0)
original = raw[section["offset"] + addr - section["address"]:
               section["offset"] + addr - section["address"] + cfg["size"]]
rows = []
for label, content in forms.items():
    cpp = OUT / (label + ".cpp")
    cpp.write_text(content)
    for snap in ("jan25", "dec06"):
        row = {"source": label, "snapshot": snap}
        obj = OUT / (label + "-" + snap + ".o")
        comp = subprocess.run(
            ["/tmp/vuc-binutils215/g++-" + snap + "-as215",
             "-O2", "-frename-registers", "-falign-jumps=1",
             "-c", str(cpp), "-o", str(obj)],
            capture_output=True, text=True,
        )
        if comp.returncode:
            row["error"] = comp.stderr[-380:]
        else:
            try:
                candidate, relocations = match.prepare_named_function(
                    obj.read_bytes(), cfg["function"], cfg["relocations"], addr,
                )
                row["size"] = len(candidate)
                row["relocations"] = relocations
                row["prefix"] = next(
                    (i for i, (a, b) in enumerate(zip(original, candidate))
                     if a != b), min(len(original), len(candidate)),
                )
                if len(candidate) == len(original):
                    score = match.run(
                        obj, VUC / "config/value_addition_partial_oct14_probe.json",
                    )
                    row["matched"] = score["exact_bytes"]
                    row["exact"] = score["exact"]
                    row["diffs"] = score["different_offsets"]
            except ValueError as exc:
                row["error"] = str(exc)
        print(json.dumps(row, sort_keys=True), flush=True)
        rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

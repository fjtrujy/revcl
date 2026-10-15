#!/usr/bin/env python3
"""Check authentic GCC 3.3 jump/label/loop alignment of Value addition.

The original vector arithmetic starts at +0xa7 in ascending lane order;
the original scalar branch starts at +0xd0 after three assembler alignment
bytes. Existing original-length source matches 203/219 with a different
lane order. Test compiler-supported alignments against both real source
orders, retaining only full 219-byte, relocated candidates.
"""
from pathlib import Path
import itertools
import json
import subprocess
import importlib.util

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/value-plus-align-interactions-oct15h")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg = VUC / "config/value_addition_partial_oct14_probe.json"
selected = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
a = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
b = """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""
assert a in selected
srcs = {"selected-3120": selected, "original-0123": selected.replace(a, b)}
rows = []
best = None
for (name, source), jump, label, loop in itertools.product(
        srcs.items(), [1, 2, 3, 4, 8], [1, 2, 4, 8, 16], [1, 2, 4]):
    tag = f"{name}-j{jump}-l{label}-loop{loop}"
    opts = ["-O2", "-frename-registers", f"-falign-jumps={jump}",
            f"-falign-labels={label}", f"-falign-loops={loop}"]
    cpp = OUT / (name + ".cpp")
    obj = OUT / (tag + ".o")
    if not cpp.exists():
        cpp.write_text(source)
    p = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts,
                        "-c", str(cpp), "-o", str(obj)],
                       text=True, capture_output=True)
    if p.returncode:
        rows.append({"tag": tag, "error": p.stderr[-130:]})
        continue
    try:
        score = matcher.run(obj, cfg)
    except ValueError as e:
        rows.append({"tag": tag, "error": str(e)})
        continue
    row = {"tag": tag, "length": score["length"], "match": score["exact_bytes"],
           "relocations": score["relocations"],
           "prefix": next(iter(score["different_offsets"]), score["length"]),
           "different_offsets": score["different_offsets"]}
    rows.append(row)
    if (row["length"], row["relocations"]) != (219, 1):
        continue
    if best is None or (row["match"], row["prefix"]) > (best["match"], best["prefix"]):
        best = row
        (OUT / "best.cpp").write_text(source)
        (OUT / "best.o").write_bytes(obj.read_bytes())
        (OUT / "best.flags").write_text(" ".join(opts) + "\n")
        print("BEST", json.dumps(row), flush=True)
    if row["match"] == 219:
        break
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
print(json.dumps({"attempted": len(rows),
                  "eligible": sum(x.get("length") == 219 and x.get("relocations") == 1 for x in rows),
                  "best": best}, indent=2))

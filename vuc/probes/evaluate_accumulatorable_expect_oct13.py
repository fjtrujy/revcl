#!/usr/bin/env python3
"""Test period GCC __builtin_expect for real accumulator branch predictions.

Original and candidate have matching instructions except padding distribution:
original 4-byte alignment before the case-12 branch plus a shorter true
block alignment, rather than candidate's single 11-byte true alignment.
This tests genuine compiler probability annotations, not padding or asm.
"""
import importlib.util
from itertools import product
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/accum-expect-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
sp = importlib.util.spec_from_file_location("m", ROOT / "vuc/scripts/match_probe.py")
m = importlib.util.module_from_spec(sp)
sp.loader.exec_module(m)
baseline = (ROOT / "vuc/probes/ir_accumulatorable.cpp").read_text()
flags = {
 "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
 "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
 "j1-label4": ["-O2", "-frename-registers", "-falign-jumps=1", "-falign-labels=4"],
 "j1-label8": ["-O2", "-frename-registers", "-falign-jumps=1", "-falign-labels=8"],
 "j1-loops4": ["-O2", "-frename-registers", "-falign-jumps=1", "-falign-loops=4"],
}
branch = [(11, "op->kind == 11"), (13, "op->kind == 13"), (12, "op->kind == 12")]
rank = []
for predictions in product(("plain", "likely", "unlikely"), repeat=3):
    source = baseline
    for (number, old), choice in zip(branch, predictions):
        if choice == "plain":
            continue
        expected = 1 if choice == "likely" else 0
        source = source.replace("if (" + old + ")", "if (__builtin_expect(" + old + ", " + str(expected) + "))")
    name = "-".join("p" if x=="plain" else "t" if x=="likely" else "f" for x in predictions)
    path = SCRATCH / (name + ".cpp")
    path.write_text(source)
    for key, opts in flags.items():
        obj = SCRATCH / (name + "-" + key + ".o")
        result = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(path), "-o", str(obj)],
                                capture_output=True, text=True)
        if result.returncode:
            print("COMPILE_FAIL", name, key, result.stderr[-120:], flush=True)
            continue
        nm = subprocess.run(["nm", "-S", str(obj)], capture_output=True, text=True).stdout
        entry = next((x for x in nm.splitlines() if "_Z17IsAccumulatorable" in x), "")
        size = int(entry.split()[1], 16) if entry else -1
        if size == 77:
            try:
                match = m.run(obj, ROOT / "vuc/config/ir_accumulatorable_probe.json")
                rank.append((match["exact_bytes"], name, key, size, match["different_offsets"]))
                if match["exact"]:
                    print("BYTE_EXACT", name, key, path, flush=True)
                    raise SystemExit(0)
            except ValueError as e:
                print("REJECT", name, key, str(e)[:110], flush=True)
        else:
            rank.append((0, name, key, size, []))
    print("checked", name, flush=True)
print("TOP")
for row in sorted(rank, key=lambda x:x[0], reverse=True)[:28]:
    print(row)

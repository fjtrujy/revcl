#!/usr/bin/env python3
"""Bounded historical-GCC non-power-of-two jump alignment control.

Try only previously unexamined values against the real SymbolTable
source-unit predecessor and the original Scope iterator-copy symbol.
Never infer byte equality from function size alone.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-binutils215/scope-iterator-nonpower-oct16")
OUT.mkdir(parents=True, exist_ok=True)
SOURCE = R / "probes/ScopeNormalIteratorRealOrderOct15t.cpp"
CONFIG = R / "config/scope_iterator_copy_oct12_probe.json"
spec = importlib.util.spec_from_file_location("match_probe", R / "scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
original = json.loads(CONFIG.read_text())
target = original["function"]
rows = []
for jumps in (2, 3, 5, 6, 7, 9, 10, 12, 16):
    obj = OUT / ("jump" + str(jumps) + ".o")
    flags = ["-O2", "-frename-registers", "-falign-jumps=" + str(jumps)]
    proc = subprocess.run(
        ["/tmp/vuc-binutils215/g++-jan25-as215", *flags,
         "-c", str(SOURCE), "-o", str(obj)],
        capture_output=True, text=True)
    row = {"jumps": jumps, "flags": flags}
    if proc.returncode:
        row["error"] = proc.stderr[-240:]
    else:
        syms = subprocess.run(["nm", "-S", "--defined-only", str(obj)],
                              capture_output=True, text=True)
        for line in syms.stdout.splitlines():
            words = line.split()
            if len(words) >= 4 and words[-1] == target:
                row["size"] = int(words[1], 16)
            if len(words) >= 4 and "_M_insert_aux" in words[-1]:
                row["neighbor_insert_size"] = int(words[1], 16)
        if row.get("size") == 149:
            try:
                score = match.run(obj, CONFIG)
                row["exact_bytes"] = score["exact_bytes"]
                row["relocations"] = score["relocations"]
                row["different_offsets"] = score["different_offsets"]
                row["exact"] = score["exact"]
                if score["exact"]:
                    (OUT / "EXACT.txt").write_text(str(jumps) + "\n")
            except ValueError as exc:
                row["error"] = str(exc)
    rows.append(row)
    print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

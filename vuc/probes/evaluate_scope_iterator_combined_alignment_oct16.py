#!/usr/bin/env python3
"""GCC 3.3.3 Scope iterator copy: combined real label/loop alignments.

The previous studies tested these constraints mostly one at a time.
This bounds their interaction in the historical source owner. Only
original-length, relocation-checked objects qualify as genuine partials.
"""
from pathlib import Path
import importlib.util
import itertools
import json
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-binutils215/scope-iterator-combined-alignment-oct16")
O.mkdir(parents=True, exist_ok=True)
SOURCE = R / "probes/ScopeNormalIteratorRealOrderOct15t.cpp"
CONFIG = R / "config/scope_iterator_copy_oct12_probe.json"
symbol = json.loads(CONFIG.read_text())["function"]
spec = importlib.util.spec_from_file_location("match_probe", R / "scripts/match_probe.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
rows = []
for jump, label, loop in itertools.product((4,), (1, 2, 4, 8), (1, 2, 4, 8)):
    # Already separately studied: skip whenever both secondary alignments
    # are 1. Test genuine interactions only, not equivalent flat repetitions.
    if label == 1 and loop == 1:
        continue
    tag = "j%d-l%d-p%d" % (jump, label, loop)
    obj = O / (tag + ".o")
    flags = ["-O2", "-frename-registers", "-falign-jumps=%d" % jump,
             "-falign-labels=%d" % label, "-falign-loops=%d" % loop]
    proc = subprocess.run(
        ["/tmp/vuc-binutils215/g++-jan25-as215", *flags,
         "-c", str(SOURCE), "-o", str(obj)], capture_output=True, text=True)
    row = {"case": tag}
    if proc.returncode:
        row["error"] = proc.stderr[-160:]
    else:
        nm = subprocess.run(["nm", "-S", "--defined-only", str(obj)],
                            capture_output=True, text=True)
        for line in nm.stdout.splitlines():
            fields = line.split()
            if len(fields) < 4:
                continue
            if fields[-1] == symbol:
                row["size"] = int(fields[1], 16)
            if "_M_insert_aux" in fields[-1]:
                row["insert_size"] = int(fields[1], 16)
        if row.get("size") == 149:
            try:
                result = mod.run(obj, CONFIG)
                row["exact_bytes"] = result["exact_bytes"]
                row["relocations"] = result["relocations"]
                row["different_offsets"] = result["different_offsets"]
                row["exact"] = result["exact"]
                if result["exact"]:
                    (O / "EXACT.txt").write_text(tag + "\n")
            except ValueError as exc:
                row["reject"] = str(exc)
    rows.append(row)
    print(json.dumps(row, sort_keys=True), flush=True)
    if row.get("exact"):
        break
(O / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

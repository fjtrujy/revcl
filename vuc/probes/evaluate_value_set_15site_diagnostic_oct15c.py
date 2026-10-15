#!/usr/bin/env python3
"""Reproduce informative *wrong-length* VUC original Value AST parser evidence.

The canonical function at 0x0806e7d0 has 455 bytes. This C++03 source
produces 459 bytes, 213 overlapping original-offset bytes, 55 exact entry
bytes, and 15 authentic named/literal reference sites using BOTH historical
GCC 3.3.3 prerelease snapshots plus GNU as 2.15. It is never selected.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

R = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_match_diag", R/"vuc/scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

config = json.loads((R/"vuc/config/value_set_constval_partial_oct15b.json").read_text())
raw, elf = m.decomp.load()
text = next(s for s in elf["sections"] if s["name"] == ".text")
addr = int(config["address"], 0)
original = raw[text["offset"] + addr - text["address"]:
               text["offset"] + addr - text["address"] + config["size"]]
out = Path("/tmp/vuc-binutils215/value-set-15site-diagnostic-oct15c")
out.mkdir(parents=True, exist_ok=True)

for snapshot in ("jan25", "dec06"):
    compiler = "/tmp/vuc-binutils215/g++-" + snapshot + "-as215"
    obj = out / ("value-set-15site-" + snapshot + ".o")
    command = [
        compiler, "-O2", "-frename-registers", "-falign-jumps=4",
        "-fno-crossjumping", "-c",
        str(R/"vuc/probes/ValueConstValSet15ReferencesDiagnosticOct15c.cpp"),
        "-o", str(obj)]
    subprocess.run(command, check=True)
    body, relocations = m.prepare_named_function(
        obj.read_bytes(), config["function"], config["relocations"], addr,
        literal_ranges=config["relocation_literals"])
    matching = sum(a == b for a, b in zip(original, body))
    prefix = next((i for i, (a, b) in enumerate(zip(original, body))
                   if a != b), min(len(body), len(original)))
    report = {
        "compiler_snapshot": snapshot,
        "original_length": config["size"],
        "candidate_length": len(body),
        "overlapping_original_bytes": matching,
        "exact_prefix_bytes": prefix,
        "resolved_reference_sites": relocations,
        "is_original_length": len(body) == config["size"],
    }
    (out / (snapshot + ".json")).write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, sort_keys=True), flush=True)
    assert (len(body), matching, prefix, relocations) == (459, 213, 55, 15)
    assert len(body) != config["size"], "Do not promote diagnostic to strict partial"

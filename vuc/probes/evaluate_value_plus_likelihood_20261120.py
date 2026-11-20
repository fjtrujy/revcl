#!/usr/bin/env python3
"""VUC Value::operator+ alignment via genuine branch-likelihood evidence.

Historical GCC 3.3.3 has different block-alignment decisions for
original-order independent vector lanes. This diagnostic tests whether
normal C++03 branches marked with the compiler's supported prediction
builtin recover the original scalar-case alignment, without adding NOPs
or changing the function's useful arithmetic.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-value-plus-likelihood-20261120")
OUT.mkdir(exist_ok=True)
selected = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
source_lanes = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
natural_lanes = """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""
assert selected.count(source_lanes) == 1
natural = selected.replace(source_lanes, natural_lanes, 1)
assert natural.count("switch (result.type)") == 3
assert natural.count("if (TypeConvert(*this, other, result, temporary))") == 3
forms = {"natural": natural, "selected": selected}
for kind in (0, 1, 3, 4):
    forms["expected_type_" + str(kind)] = natural.replace(
        "switch (result.type)",
        "switch (__builtin_expect(result.type, " + str(kind) + "))",
        1,
    )
for yes in (0, 1):
    forms["expected_convert_" + str(yes)] = natural.replace(
        "if (TypeConvert(*this, other, result, temporary))",
        "if (__builtin_expect(TypeConvert(*this, other, result, temporary), "
        + str(yes) + "))",
        1,
    )
    for kind in (3, 4):
        forms["type_" + str(kind) + "_convert_" + str(yes)] = forms[
            "expected_type_" + str(kind)
        ].replace(
            "if (TypeConvert(*this, other, result, temporary))",
            "if (__builtin_expect(TypeConvert(*this, other, result, temporary), "
            + str(yes) + "))",
            1,
        )

spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
config = VUC / "config/value_addition_partial_oct14_probe.json"
config_data = json.loads(config.read_text())
reference_raw, reference_elf = match.decomp.load()
sec = next(s for s in reference_elf["sections"] if s["name"] == ".text")
start = sec["offset"] + int(config_data["address"], 0) - sec["address"]
original = reference_raw[start:start + config_data["size"]]
rows = []
for tag, content in forms.items():
    src = OUT / (tag + ".cpp")
    src.write_text(content)
    for compiler in ("jan25", "dec06"):
        row = {"variant": tag, "compiler": compiler}
        obj = OUT / (tag + "-" + compiler + ".o")
        cp = subprocess.run(
            [
                "/tmp/vuc-binutils215/g++-" + compiler + "-as215",
                "-O2", "-frename-registers", "-falign-jumps=1",
                "-c", str(src), "-o", str(obj),
            ],
            capture_output=True, text=True,
        )
        if cp.returncode:
            row["error"] = cp.stderr[-300:]
        else:
            try:
                resolved, relocations = match.prepare_named_function(
                    obj.read_bytes(), config_data["function"],
                    config_data["relocations"], int(config_data["address"], 0),
                )
                row["length"] = len(resolved)
                row["relocations"] = relocations
                row["prefix"] = next(
                    (i for i, (a, b) in enumerate(zip(original, resolved))
                     if a != b),
                    min(len(original), len(resolved)),
                )
                if len(resolved) == len(original):
                    comparison = match.run(obj, config)
                    row["matched"] = comparison["exact_bytes"]
                    row["differences"] = comparison["different_offsets"]
                    row["exact"] = comparison["exact"]
            except ValueError as exc:
                row["error"] = str(exc)
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

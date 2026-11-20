#!/usr/bin/env python3
"""Test real Value division successor as an additional original TU owner.

VCL rebuilds recovered register/label decisions by joining real functions,
not synthetic padding. Original VUC Value.cpp emits plus, minus, multiply,
division in address order. Check the real division source's effect on its
near-exact addition predecessor, preserving the exact two siblings.
"""
from pathlib import Path
import json
import importlib.util
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-value-division-successor-20261120")
OUT.mkdir(exist_ok=True)
addition = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
division_src = (VUC / "probes/ValueDivisionUnit.cpp").read_text()
start = division_src.index("Value Value::operator/(const Value &other) const")
division = division_src[start:]
assert addition.count("    Value operator-(const Value &other) const;") == 1
assert addition.count("    Value operator*(const Value &other) const;") == 1
assert addition.count("Value Value::operator-(const Value &other) const") == 1
assert addition.count("Value Value::operator*(const Value &other) const") == 1
old_lanes = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
new_lanes = """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""
assert addition.count(old_lanes) == 1

forms = {
    "selected-original": addition,
    "natural-original": addition.replace(old_lanes, new_lanes, 1),
}
for lane, code in list(forms.items()):
    code = code.replace(
        "    Value operator-(const Value &other) const;",
        "    Value operator/(const Value &other) const;\n"
        "    Value operator-(const Value &other) const;",
        1,
    )
    forms[lane + "-div-after-mul"] = code + "\n" + division
    forms[lane + "-div-before-mul"] = code.replace(
        "Value Value::operator*(const Value &other) const\n",
        division + "\nValue Value::operator*(const Value &other) const\n",
        1,
    )

targets = {
    "plus": "value_addition_partial_oct14_probe.json",
    "minus": "value_arithmetic_minus_probe.json",
    "multiply": "value_multiply_exact_oct13.json",
    "divide": "value_division_partial_oct13.json",
}
spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
rows = []
for label, source in forms.items():
    cpp = OUT / (label + ".cpp")
    cpp.write_text(source)
    for snap in ("jan25", "dec06"):
        row = {"case": label, "compiler": snap}
        obj = OUT / (label + "-" + snap + ".o")
        comp = subprocess.run(
            ["/tmp/vuc-binutils215/g++-" + snap + "-as215",
             "-O2", "-frename-registers", "-falign-jumps=1",
             "-c", str(cpp), "-o", str(obj)],
            text=True, capture_output=True,
        )
        if comp.returncode:
            row["error"] = comp.stderr[-400:]
        else:
            for name, cfg in targets.items():
                try:
                    s = m.run(obj, VUC / "config" / cfg)
                    row[name] = {
                        "matched": s["exact_bytes"],
                        "length": s["length"],
                        "exact": s["exact"],
                        "relocs": s["relocations"],
                        "offsets": s["different_offsets"][:20],
                    }
                except ValueError as exc:
                    row[name] = {"error": str(exc)}
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

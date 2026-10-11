#!/usr/bin/env python3
"""Explore original-era Value multiplication TU context (no register forcing).

The difference from the original currently consists of only three x86 register
encoding bytes in the integer multiplication case. Every candidate still uses
normal C++ operators and the canonical exact relocation-aware matcher.
"""
import importlib.util
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[2]
T = Path("/tmp/vuc-binutils215")
spec = importlib.util.spec_from_file_location("vuc_match", R / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
source = (R / "vuc/probes/value_ml.cpp").read_text()
begin = source.index("Value Value::operator+(")
mul_at = source.index("Value Value::operator*(")
header, plus, multiply = source[:begin], source[begin:mul_at], source[mul_at:]

contexts = {
    "both-original": plus + multiply,
    "mul-first": multiply + plus,
    "mul-only": multiply,
    "plus-twice": plus + multiply + plus.replace("operator+", "other_added_method"),
}
integer_lines = {
    "compound": "result.bits *= temporary.bits;",
    "result-left": "result.bits = result.bits * temporary.bits;",
    "temp-left": "result.bits = temporary.bits * result.bits;",
    "signed": "result.bits = (int)result.bits * (int)temporary.bits;",
    "tmp-local": "unsigned rhs = temporary.bits; result.bits *= rhs;",
    "lhs-local": "unsigned lhs = result.bits; result.bits = lhs * temporary.bits;",
}
needle = "result.bits *= temporary.bits;"
assert needle in multiply
rows = []
for context_name, bodies in contexts.items():
    if context_name == "plus-twice":
        # A third member is an explicitly named normal source-level method.
        declaration = "    Value other_added_method(const Value &other) const;\n"
    else:
        declaration = ""
    for line_name, replacement in integer_lines.items():
        content = header.replace(
            "    Value operator+(const Value &other) const;",
            "    Value operator+(const Value &other) const;\n" + declaration
        ) + bodies.replace(needle, replacement)
        path = T / ("mul-context-" + context_name + "-" + line_name + ".cpp")
        path.write_text(content)
        for mode, flags in (
            ("ren-j1", ["-O2", "-frename-registers", "-falign-jumps=1"]),
            ("j1", ["-O2", "-falign-jumps=1"]),
        ):
            obj = path.with_suffix("." + mode + ".o")
            proc = subprocess.run([str(T / "g++-jan25-as215"), *flags, "-c",
                                   str(path), "-o", str(obj)],
                                  text=True, capture_output=True)
            if proc.returncode:
                rows.append((0, context_name, line_name, mode, proc.stderr[-100:]))
                continue
            try:
                result = match.run(obj, R / "vuc/config/value_ml_probe.json")
                rows.append((result["exact_bytes"], context_name, line_name,
                             mode, str(result["length"])))
                if result["exact"]:
                    print("NEW EXACT", context_name, line_name, mode, flush=True)
            except ValueError as exc:
                rows.append((0, context_name, line_name, mode, str(exc)[:100]))
for item in sorted(rows, reverse=True)[:30]:
    print(*item)

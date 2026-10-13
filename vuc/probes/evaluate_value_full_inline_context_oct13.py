#!/usr/bin/env python3
"""Test Value multiply in the genuine recovered sibling-method source context.

Value.cpp currently exposes a separately defined constructor; the recovered
integer/float arithmetic originally inlined that simple constructor.
Compare normal inline constructor declaration and original member order with
strict relocation verification and no assembly/register forcing.
"""
from pathlib import Path
import importlib.util
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/value-inline-source-unit-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
unit = (ROOT / "vuc/src/Value.cpp").read_text()
assert "    Value();" in unit
constructor = "Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}"
assert constructor in unit
inline = "Value() : type(0), a(0), b(0), flag0(0), flag1(0), flag2(0), flag3(0) {}"
inline_unit = unit.replace("    Value();", "    " + inline).replace(constructor, "")
plus_begin = inline_unit.index("Value Value::operator+(")
mul_begin = inline_unit.index("Value Value::operator*(")
next_begin = inline_unit.index("Value Value::operator==(")
variants = {
    "full-inline": inline_unit,
    "full-out-of-line": unit,
    "full-inline-mul-first": (
        inline_unit[:plus_begin] + inline_unit[mul_begin:next_begin]
        + inline_unit[plus_begin:mul_begin] + inline_unit[next_begin:]
    ),
}
flags = {
    "ren-j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "ren": ["-O2", "-frename-registers"],
    "plain-j1": ["-O2", "-falign-jumps=1"],
    "ren-j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "rename-sections": ["-O2", "-frename-registers", "-falign-jumps=1", "-ffunction-sections"],
}
for name, source in variants.items():
    source_file = SCRATCH / (name + ".cpp")
    source_file.write_text(source)
    for f_name, opts in flags.items():
        object_file = SCRATCH / (name + "-" + f_name + ".o")
        result = subprocess.run(
            ["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(source_file), "-o", str(object_file)],
            capture_output=True, text=True)
        if result.returncode:
            print("COMPILE_FAIL", name, f_name, result.stderr[-170:], flush=True)
            continue
        try:
            score = matcher.run(object_file, ROOT / "vuc/config/value_ml_probe.json")
            print(name, f_name, "MATCH", score["exact_bytes"], score["length"],
                  score["different_offsets"], flush=True)
            if score["exact"]:
                print("BYTE_EXACT", source_file, f_name, flush=True)
                raise SystemExit(0)
        except ValueError as error:
            print(name, f_name, "REJECT", str(error)[:130], flush=True)

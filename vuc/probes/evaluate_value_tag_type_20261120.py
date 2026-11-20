#!/usr/bin/env python3
"""VUC Value arithmetic: test historically plausible 32-bit tag types.

VCL decompilation showed that changing signedness/type identity can
change GCC branch-frequency and code layout even with identical emitted
operations. Recover real VUC Value tag declaration (possibly TypeID enum)
as a source hypothesis; protect exact subtraction/multiplication siblings.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-value-tag-type-20261120")
OUT.mkdir(exist_ok=True)
base = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
old_lanes = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
new_lanes = """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""
assert base.count(old_lanes) == 1 and base.count("    int type;") == 1
forms = {
    "int": ("int", None),
    "unsigned": ("unsigned int", None),
    "long": ("long", None),
    "unsigned_long": ("unsigned long", None),
    "enum": ("ValueTag", "enum ValueTag { Zero = 0, Integer = 1, Scalar = 3, Vector = 4 };"),
    "const_int": ("const int", None),
}
spec = importlib.util.spec_from_file_location("match", VUC / "scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
targets = {
    "plus": "value_addition_partial_oct14_probe.json",
    "minus": "value_arithmetic_minus_probe.json",
    "multiply": "value_multiply_exact_oct13.json",
}
rows = []
for tag, (type_name, declaration) in forms.items():
    for lane, code in (("selected", base), ("natural", base.replace(old_lanes, new_lanes, 1))):
        src = code.replace("    int type;", "    " + type_name + " type;", 1)
        if declaration is not None:
            src = declaration + "\n" + src
            src = src.replace("Value() : type(0)", "Value() : type(Zero)", 1)
        path = OUT / (tag + "-" + lane + ".cpp")
        path.write_text(src)
        for snapshot in ("jan25", "dec06"):
            row = {"tag": tag, "lanes": lane, "compiler": snapshot}
            obj = OUT / (tag + "-" + lane + "-" + snapshot + ".o")
            proc = subprocess.run([
                "/tmp/vuc-binutils215/g++-" + snapshot + "-as215",
                "-O2", "-frename-registers", "-falign-jumps=1",
                "-c", str(path), "-o", str(obj),
            ], capture_output=True, text=True)
            if proc.returncode:
                row["compile_error"] = proc.stderr[-280:]
            else:
                for name, cfg in targets.items():
                    try:
                        result = match.run(obj, VUC / "config" / cfg)
                        row[name] = {
                            "match": result["exact_bytes"],
                            "length": result["length"],
                            "reloc": result["relocations"],
                            "exact": result["exact"],
                        }
                    except ValueError as exc:
                        row[name] = {"error": str(exc)}
            rows.append(row)
            print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

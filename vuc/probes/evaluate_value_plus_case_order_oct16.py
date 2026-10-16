#!/usr/bin/env python3
"""Historical C++03 Value-addition case order and tail-sharing controls.

Test only ordinary source, with the original length and actual ELF
relocations checked by match_probe.py. No emitted bytes are patched.
"""
from pathlib import Path
import importlib.util
import subprocess

R = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-valueplus-caseorder-oct16")
OUT.mkdir(exist_ok=True)
source = (R / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
start = source.index("        switch (result.type) {")
end = source.index("\n        }", start) + len("\n        }")
prefix, suffix = source[:start], source[end:]
scalar = "case 3: result.values[0] += temporary.values[0]; break;"
integer = "case 1: result.bits += temporary.bits; break;"
vector = """case 4:
            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];
            break;"""
vector_ascending = """case 4:
            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];
            break;"""
forms = {
    "baseline": [integer, scalar, vector],
    "case143": [integer, vector, scalar],
    "case413": [vector, integer, scalar],
    "case431": [vector, scalar, integer],
    "case314": [scalar, integer, vector],
    "case341": [scalar, vector, integer],
    "ascending143": [integer, vector_ascending, scalar],
    "ascending413": [vector_ascending, integer, scalar],
    "ascending431": [vector_ascending, scalar, integer],
    "ascending314": [scalar, integer, vector_ascending],
}
spec = importlib.util.spec_from_file_location("match_probe", R / "scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
best = (203, "baseline")
for name, cases in forms.items():
    path = OUT / (name + ".cpp")
    path.write_text(prefix + "        switch (result.type) {\n" +
                    "\n".join("        " + case for case in cases) +
                    "\n        }" + suffix)
    for compiler in ("jan25",):
        obj = OUT / (name + "-" + compiler + ".o")
        cmd = ["/tmp/vuc-binutils215/g++-" + compiler + "-as215",
               "-O2", "-frename-registers", "-falign-jumps=1",
               "-c", str(path), "-o", str(obj)]
        run = subprocess.run(cmd, capture_output=True, text=True)
        if run.returncode:
            print("COMPILE_FAIL", name, run.stderr[-180:], flush=True)
            continue
        try:
            result = match.run(obj, R / "config/value_addition_partial_oct14_probe.json")
        except Exception as exc:
            print("REJECT", name, str(exc)[:220], flush=True)
            continue
        score = result["exact_bytes"]
        print("RESULT", name, compiler, score, result["length"],
              result["different_offsets"][:25], flush=True)
        if score > best[0] and result["length"] == 219:
            best = (score, name)
            (OUT / "BEST").write_text(name + " " + str(score) + "\n")
        if result["exact"]:
            (OUT / "EXACT").write_text(name + "\n")
            print("BYTE_EXACT", name, flush=True)
print("BEST", best, flush=True)

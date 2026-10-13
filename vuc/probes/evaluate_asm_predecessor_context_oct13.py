#!/usr/bin/env python3
"""VUC AsmOutput predecessor-context controls under historical GCC 3.3.3.

Vary only genuine iostream expression forms in the two original predecessors.
Compare AlignString, AlignConstant, TabConstants and both predecessors, including
all 127 original literal bytes. No generated instruction bytes are patched.
"""
import importlib.util
import itertools
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/asm-predecessor-context-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("match", ROOT / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
base = (ROOT / "vuc/src/AsmOutputUnit.cpp").read_text()
# The string is split into adjacent literal tokens in the real source.
begin = base.index("void AsmOutput_WritePrologue(")
end = base.index("void AsmOutput_WriteEpilogue(", begin)
ep_end = base.index("#include <cstring>", end)
prologue_body = base[begin:end]
epilogue_body = base[end:ep_end]
p_expr = prologue_body[prologue_body.index('    out <<'):prologue_body.rindex(';') + 1]
e_expr = epilogue_body[epilogue_body.index('    out <<'):epilogue_body.rindex(';') + 1]
assert p_expr.startswith('    out <<') and e_expr.startswith('    out <<')
def forms(expr):
    literal = expr[expr.index('<<') + 2:expr.rindex(';')].strip()
    return {
        "direct": expr,
        "std-call": "    std::operator<<(out, " + literal + ");",
        "ostream-ref": "    std::ostream &stream = out;\n    stream << " + literal + ";",
        "text-local": "    const char *text = " + literal + ";\n    out << text;",
    }

full_literals = [
    {"offset": 0, "size": 92, "address": "0x080720a0"},
    {"offset": 92, "size": 19, "address": "0x080721c4"},
    {"offset": 111, "size": 3, "address": "0x080721d7"},
    {"offset": 114, "size": 8, "address": "0x080721f0"},
    {"offset": 122, "size": 5, "address": "0x080721fb"},
]
configs = {}
for tag, filename in {
    "prologue": "asm_output_unit_oct13_probe.json",
    "epilogue": "asm_output_epilogue_probe.json",
    "align": "align_string_probe.json",
    "constant": "align_constant_exact_probe.json",
    "tab": "asm_output_tab_constants_probe.json",
}.items():
    cfg = json.loads((ROOT / "vuc/config" / filename).read_text())
    cfg["relocation_literals"] = {".rodata": full_literals}
    cfg["relocations"].pop(".rodata", None)
    cfg_path = SCRATCH / (tag + ".json")
    cfg_path.write_text(json.dumps(cfg))
    configs[tag] = cfg_path
flags = {
    "base": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "plain": ["-O2", "-falign-jumps=4"],
    "no-sched2": ["-O2", "-frename-registers", "-falign-jumps=4", "-fno-schedule-insns2"],
    "no-regmove": ["-O2", "-frename-registers", "-falign-jumps=4", "-fno-regmove"],
}
results = []
for (p_name, p), (e_name, e) in itertools.product(forms(p_expr).items(), forms(e_expr).items()):
    source = base.replace(p_expr, p, 1).replace(e_expr, e, 1)
    src = SCRATCH / (p_name + "-" + e_name + ".cpp")
    src.write_text(source)
    for flagname, opts in flags.items():
        obj = SCRATCH / (p_name + "-" + e_name + "-" + flagname + ".o")
        command = ["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(src), "-o", str(obj)]
        build = subprocess.run(command, capture_output=True, text=True)
        if build.returncode:
            print("COMPILE_FAIL", src.name, flagname, build.stderr[-200:], flush=True)
            continue
        scores = {}
        for tag, config in configs.items():
            try:
                answer = match.run(obj, config)
                scores[tag] = answer["exact_bytes"]
            except ValueError:
                scores[tag] = -1
        score = (scores["align"], scores["prologue"], scores["epilogue"])
        results.append((score, p_name, e_name, flagname, scores))
        if scores["align"] == 145 or scores["prologue"] == 30:
            print("PROMISING", p_name, e_name, flagname, scores, flush=True)
        if scores["align"] == 145 and scores["constant"] == 69 and scores["tab"] == 127:
            print("ALIGN_EXACT", src, flagname, flush=True)
print("TOP_CONTEXT_VARIANTS")
for record in sorted(results, reverse=True)[:18]:
    print(record)

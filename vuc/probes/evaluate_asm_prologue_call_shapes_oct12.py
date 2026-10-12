#!/usr/bin/env python3
"""Natural C++ stream insertion forms for VUC's 30-byte AsmOutput prologue."""
import importlib.util
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TMP = Path("/tmp/vuc-binutils215/asm-prologue-oct12")
TMP.mkdir(parents=True, exist_ok=True)
SPEC = importlib.util.spec_from_file_location("match", ROOT / "vuc/scripts/match_probe.py")
MATCH = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MATCH)
base = (ROOT / "vuc/probes/asm_output_hypothesis.cpp").read_text()
literal = base.split("out << ", 1)[1].rsplit(";", 1)[0].strip()
config = json.loads((ROOT / "vuc/config/asm_output_probe.json").read_text())
config["relocations"][".rodata"] = config["relocations"].pop(".rodata.str1.32")
config_path = TMP / "probe-config.json"
config_path.write_text(json.dumps(config))
forms = {
 "member": "out << MSG;",
 "function": "std::operator<<(out, MSG);",
 "parenthesized": "(out << MSG);",
 "local-text": "const char *text = MSG; out << text;",
 "local-function": "const char *text = MSG; std::operator<<(out, text);",
 "base-member": "std::ostream &stream = out; stream << MSG;",
 "base-function": "std::ostream &stream = out; std::operator<<(stream, MSG);",
 "pointer-member": "std::ofstream *ptr = &out; (*ptr) << MSG;",
 "pointer-function": "std::ofstream *ptr = &out; std::operator<<(*ptr, MSG);",
 "base-pointer": "std::ostream *stream = &out; std::operator<<(*stream, MSG);",
 "const-pointer": "const char *const text = MSG; out << text;",
}
flags = {
 "o2": ["-O2"],
 "rename": ["-O2", "-frename-registers"],
 "no-sched2": ["-O2", "-fno-schedule-insns2"],
 "rename-j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
}
results = []
for name, body in forms.items():
    src = TMP / (name + ".cpp")
    src.write_text("#include <fstream>\nvoid AsmOutput_WritePrologue(std::ofstream &out) {\n" + body.replace("MSG", literal) + "\n}\n")
    for flagname, opts in flags.items():
        obj = TMP / (name + "-" + flagname + ".o")
        cp = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(src), "-o", str(obj)], capture_output=True, text=True)
        if cp.returncode:
            results.append((0, name, flagname, "compile: " + cp.stderr[-80:]))
            continue
        try:
            result = MATCH.run(obj, config_path)
            record = (result["exact_bytes"], name, flagname, result["length"], result["different_offsets"])
            results.append(record)
            if result["exact"]:
                print("EXACT", record)
                raise SystemExit(0)
        except ValueError as err:
            results.append((0, name, flagname, str(err)[:100]))
for row in sorted(results, reverse=True)[:25]:
    print(*row)

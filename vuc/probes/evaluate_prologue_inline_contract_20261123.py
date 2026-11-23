#!/usr/bin/env python3
"""Probe authentic C++03 inline ostream helper contracts for VUC's prologue.

The original 30-byte function loads the literal in EAX and the receiver in
EDX; baseline GCC 3.3.3 reuses EAX for the receiver and schedules it later.
Inline stream/text API wrappers may affect genuine frontend pseudo-register
liveness without changing the final operation. Never alter compiled bytes.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-prologue-inline-contract-20261123")
OUT.mkdir(exist_ok=True)
original = (VUC / "probes/asm_output_hypothesis.cpp").read_text()
prefix, suffix = original.split("void AsmOutput_WritePrologue(std::ofstream &out)", 1)
argument = suffix.split("out <<", 1)[1].rsplit(";", 1)[0].strip()
helpers = """
template <typename T> inline T &same(T &value) { return value; }
template <typename T> inline T *samePtr(T *value) { return value; }
inline std::ostream &asStream(std::ofstream &value) { return value; }
template <typename T> inline std::ostream &asStreamT(T &value) { return value; }
inline const char *asText(const char *value) { return value; }
template <typename T> inline const T &sameConst(const T &value) { return value; }
inline void emitStream(std::ofstream &value, const char *message) { value << message; }
inline void emitOutput(std::ostream &value, const char *message) { value << message; }
template <typename T> inline void emitTemplate(T &value, const char *message) {
    value << message;
}
"""
variants = {
    "baseline": "out << STRING;",
    "same-stream": "same(out) << STRING;",
    "same-stream-explicit": "std::operator<<(same(out), STRING);",
    "stream-pointer": "(*samePtr(&out)) << STRING;",
    "std-stream": "asStream(out) << STRING;",
    "std-stream-explicit": "std::operator<<(asStream(out), STRING);",
    "std-stream-template": "asStreamT(out) << STRING;",
    "std-stream-template-explicit": "std::operator<<(asStreamT(out), STRING);",
    "text-wrapper": "out << asText(STRING);",
    "text-wrapper-explicit": "std::operator<<(out, asText(STRING));",
    "text-template-const": "out << sameConst(STRING);",
    "both-templated": "same(out) << sameConst(STRING);",
    "pass-template": "emitTemplate(out, STRING);",
    "pass-ofstream": "emitStream(out, STRING);",
    "pass-ostream": "emitOutput(out, STRING);",
    "cast-void-ostream": "return (void)(asStream(out) << STRING);",
    "cast-void-template": "return (void)(same(out) << STRING);",
}
matcher_spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(matcher_spec)
matcher_spec.loader.exec_module(matcher)
cfg = VUC / "config/asm_output_jan25_probe.json"
rows = []
for tag, snippet in variants.items():
    source = prefix + helpers + "\nvoid AsmOutput_WritePrologue(std::ofstream &out)\n{\n    "
    source += snippet.replace("STRING", argument) + "\n}\n"
    src = OUT / (tag + ".cpp")
    src.write_text(source)
    for snapshot in ("jan25", "dec06"):
        out = OUT / (tag + "-" + snapshot + ".o")
        row = {"variant": tag, "snapshot": snapshot}
        p = subprocess.run(["/tmp/vuc-binutils215/g++-" + snapshot + "-as215",
                            "-O2", "-c", str(src), "-o", str(out)],
                           capture_output=True, text=True)
        if p.returncode:
            row["compile_error"] = p.stderr[-350:]
        else:
            try:
                result = matcher.run(out, cfg)
                row.update(length=result["length"], matched=result["exact_bytes"],
                           diffs=result["different_offsets"],
                           relocations=result["relocations"], exact=result["exact"])
            except (OSError, ValueError) as e:
                row["unverified"] = str(e)
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

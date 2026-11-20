#!/usr/bin/env python3
"""Test original-era return contracts for VUC's 30-byte ostream prologue.

The Itanium mangled name does not encode a non-template function's return
type. These source forms distinguish whether a genuine returned stream
reference changes GCC 3.3.3's register dataflow and byte sequence.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-prologue-return-contract-20261120")
OUT.mkdir(parents=True, exist_ok=True)
base = (VUC / "probes/asm_output_hypothesis.cpp").read_text()
old_signature = "void AsmOutput_WritePrologue(std::ofstream &out)"
old_expression = "    out <<"
assert old_signature in base and old_expression in base
cases = {
    "void_original": base,
    "ostream_reference_return": base.replace(
        old_signature, "std::ostream &AsmOutput_WritePrologue(std::ofstream &out)"
    ).replace(old_expression, "    return out <<"),
    "ostream_const_reference_return": base.replace(
        old_signature, "const std::ostream &AsmOutput_WritePrologue(std::ofstream &out)"
    ).replace(old_expression, "    return out <<"),
    "ofstream_reference_return": base.replace(
        old_signature, "std::ofstream &AsmOutput_WritePrologue(std::ofstream &out)"
    ).replace("\n}\n", "\n    return out;\n}\n"),
    "void_stream_alias": base.replace(
        old_expression, "    std::ostream &stream = out;\n    stream <<"),
    "void_pointer_alias": base.replace(
        old_expression, "    std::ostream *stream = &out;\n    *stream <<"),
}
spec = importlib.util.spec_from_file_location(
    "vuc_match_probe", VUC / "scripts/match_probe.py"
)
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)
config = VUC / "config/asm_output_jan25_probe.json"
rows = []
for label, source in cases.items():
    src = OUT / (label + ".cpp")
    src.write_text(source)
    for snapshot in ("jan25", "dec06"):
        obj = OUT / (label + "-" + snapshot + ".o")
        cmd = [
            "/tmp/vuc-binutils215/g++-" + snapshot + "-as215",
            "-O2", "-c", str(src), "-o", str(obj),
        ]
        result = subprocess.run(cmd, text=True, capture_output=True)
        row = {"case": label, "snapshot": snapshot}
        if result.returncode:
            row["error"] = result.stderr[-500:]
        else:
            try:
                score = checker.run(obj, config)
                row.update(
                    length=score["length"],
                    matched=score["exact_bytes"],
                    exact=score["exact"],
                    relocations=score["relocations"],
                    literals=score["verified_literal_bytes"],
                    differences=score["different_offsets"],
                )
            except ValueError as exc:
                row["error"] = str(exc)
        print(json.dumps(row, sort_keys=True), flush=True)
        rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

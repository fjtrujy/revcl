#!/usr/bin/env python3
"""Diagnostic GCC 3.3.5 comparison for VUC 2004 prerelease near matches.

This is a *noncanonical* provenance control: GNU as 2.15 remains fixed,
while GCC changes from the recovered 20040125 prerelease to Debian GCC 3.3.5.
Never promote a candidate on the basis of noncanonical compiler output.
"""
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-compiler-version-control-20261120")
OUT.mkdir(exist_ok=True)
spec = importlib.util.spec_from_file_location("probe", VUC / "scripts/match_probe.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)

original_scope = json.loads(
    (VUC / "config/hash_scope_uninitialized_probe.json").read_text()
)
original_scope.update(
    function=(
        "_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iterator"
        "IP11MLHashTableI12MLGameStringP16SymbolTableEntry"
        "9MLHashFunIS3_ELi513EESt6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type"
    ),
    address="0x0806a840", size=149, source="vuc/src/SymbolTableUnit.cpp",
)
scope_cfg = OUT / "scope-original.json"
scope_cfg.write_text(json.dumps(original_scope, indent=2) + "\n")
source = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
old = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
new = """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""
assert source.count(old) == 1
natural_path = OUT / "value-natural.cpp"
natural_path.write_text(source.replace(old, new, 1))
targets = (
    ("scope", VUC / "src/SymbolTableUnit.cpp", scope_cfg,
     ["-O2", "-frename-registers", "-falign-jumps=1"]),
    ("value-selected", VUC / "probes/ValueAdditionLaneOrderOct15.cpp",
     VUC / "config/value_addition_partial_oct14_probe.json",
     ["-O2", "-frename-registers", "-falign-jumps=1"]),
    ("value-natural", natural_path,
     VUC / "config/value_addition_partial_oct14_probe.json",
     ["-O2", "-frename-registers", "-falign-jumps=1"]),
    ("printer", VUC / "src/Printer.cpp",
     VUC / "config/print_ast_fun_context_oct14_probe.json", ["-O2"]),
    ("cleanup", VUC / "probes/instruction_list_arena_unit_oct14.cpp",
     VUC / "config/instruction_arena_cleanup_unit_oct14_probe.json",
     ["-O2", "-frename-registers", "-fuse-cxa-atexit"]),
)
assem = "/home/fjtrujy.guest/.cache/vuc-toolchain/binutils215/tool-bin/"
compilers = {
    "jan25": "/tmp/vuc-binutils215/g++-jan25-as215",
    "debian335": "/tmp/vuc-gcc335/g++-compat",
}
rows = []
for key, src, cfg, flags in targets:
    manifest = json.loads(cfg.read_text())
    for label, compiler in compilers.items():
        row = {"target": key, "compiler": label}
        obj = OUT / (key + "-" + label + ".o")
        cmd = [compiler]
        if label == "debian335":
            cmd.extend(["-B" + assem])
        cmd.extend(flags + ["-c", str(src), "-o", str(obj)])
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode:
            row["compile_error"] = res.stderr[-300:]
        else:
            try:
                code, n_reloc = probe.prepare_named_function(
                    obj.read_bytes(), manifest["function"], manifest["relocations"],
                    int(manifest["address"], 0),
                    manifest.get("relocation_objects", {}),
                    manifest.get("relocation_literals", {}),
                    manifest.get("compiler_generated_unit"),
                )
                row["length"] = len(code)
                row["relocations"] = n_reloc
                if len(code) == manifest["size"]:
                    score = probe.run(obj, cfg)
                    row["matched"] = score["exact_bytes"]
                    row["exact"] = score["exact"]
                    row["diffs"] = score["different_offsets"][:25]
            except ValueError as exc:
                row["error"] = str(exc)
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

#!/usr/bin/env python3
"""Test STL exception-cleanup loop frequency without changing the normal path.

VCL exact-match research demonstrated that a distant cold-block frequency
can change GCC's conditional label alignment. Only the *real* historical
std::__destroy_aux loop is varied in a scratch copy of an authenticated header.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-scope-cleanup-likelihood-20261120")
HEADER = Path("/tmp/vuc-gcc335/root/usr/include/c++/3.3/bits/stl_construct.h")
HASH = "6970c64a416d9a5048f75caed1028c2264bf9d8ccde14941d840c138093281d7"
OUT.mkdir(exist_ok=True)
historical = HEADER.read_text()
assert hashlib.sha256(HEADER.read_bytes()).hexdigest() == HASH
needle = "for ( ; __first != __last; ++__first) _Destroy(&*__first);"
assert historical.count(needle) == 1

forms = {
    "historical": needle,
    "cleanup_likely": (
        "for ( ; __builtin_expect(__first != __last, 1); ++__first)"
        " _Destroy(&*__first);"
    ),
    "cleanup_unlikely": (
        "for ( ; __builtin_expect(__first != __last, 0); ++__first)"
        " _Destroy(&*__first);"
    ),
    "cleanup_end_likely": (
        "for ( ; !__builtin_expect(__first == __last, 1); ++__first)"
        " _Destroy(&*__first);"
    ),
    "cleanup_end_unlikely": (
        "for ( ; !__builtin_expect(__first == __last, 0); ++__first)"
        " _Destroy(&*__first);"
    ),
}
spec = importlib.util.spec_from_file_location("probe", VUC / "scripts/match_probe.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)
manifest = json.loads((VUC / "config/hash_scope_uninitialized_probe.json").read_text())
manifest.update(
    function=(
        "_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iterator"
        "IP11MLHashTableI12MLGameStringP16SymbolTableEntry"
        "9MLHashFunIS3_ELi513EESt6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type"
    ),
    address="0x0806a840", size=149, source="vuc/src/SymbolTableUnit.cpp",
)
manifest_path = OUT / "manifest.json"
manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
rows = []
for label, replacement in forms.items():
    inc = OUT / label
    header = inc / "bits/stl_construct.h"
    header.parent.mkdir(parents=True, exist_ok=True)
    header.write_text(historical.replace(needle, replacement, 1))
    for snap in ("jan25", "dec06"):
        row = {"label": label, "compiler": snap}
        obj = OUT / (label + "-" + snap + ".o")
        cp = subprocess.run([
            "/tmp/vuc-binutils215/g++-" + snap + "-as215",
            "-O2", "-frename-registers", "-falign-jumps=1",
            "-I", str(inc), "-c",
            str(VUC / "src/SymbolTableUnit.cpp"), "-o", str(obj),
        ], capture_output=True, text=True)
        if cp.returncode:
            row["error"] = cp.stderr[-350:]
        else:
            symbols = subprocess.run(
                ["nm", "-S", "--defined-only", str(obj)],
                capture_output=True, text=True, check=True,
            ).stdout
            target = next((
                line.split() for line in symbols.splitlines()
                if line.endswith(manifest["function"])
            ), None)
            row["length"] = int(target[1], 16) if target else None
            try:
                result = probe.run(obj, manifest_path)
                row["matched"] = result["exact_bytes"]
                row["relocations"] = result["relocations"]
                row["exact"] = result["exact"]
                row["differences"] = result["different_offsets"]
            except ValueError as exc:
                row["score_error"] = str(exc)
            for name, cfg in (
                ("find", "symboltable_find_probe.json"),
                ("begin", "symbol_begin_scope_probe.json"),
                ("end", "symboltable_end_exact_probe.json"),
                ("findcopy", "hash_symbol_findcopy_probe.json"),
            ):
                try:
                    row[name] = probe.run(obj, VUC / "config" / cfg)["exact"]
                except ValueError as exc:
                    row[name] = str(exc)[:75]
        print(json.dumps(row, sort_keys=True), flush=True)
        rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

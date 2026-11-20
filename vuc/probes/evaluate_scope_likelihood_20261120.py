#!/usr/bin/env python3
"""GCC 3.3.3 VUC Scope helper: inspect real STL loop-likelihood effects.

Work entirely in the guest scratch directory. This diagnostic copies the
SHA-pinned historical libstdc++ header rather than modifying a compiler or
accepted VUC source. Never promote header probes as matching source.
"""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-scope-likelihood-20261120")
HEADER = Path("/tmp/vuc-gcc335/root/usr/include/c++/3.3/bits/stl_uninitialized.h")
HASH = "f9b0205b994016afd4570fb18bb0144acf213b31dcb23e9ae1748d61a59342a0"
OUT.mkdir(exist_ok=True)
original_header = HEADER.read_text()
assert hashlib.sha256(HEADER.read_bytes()).hexdigest() == HASH
needle = "for ( ; __first != __last; ++__first, ++__cur)"
assert original_header.count(needle) == 1

# The only changed predicate is the actual STL uninitialized-copy loop.
# A source-level likelihood hint can alter CFG frequencies without inserting
# dummy instructions, changing signatures, or fabricating addresses.
forms = {
    "historical": needle,
    "loop_expected": "for ( ; __builtin_expect(__first != __last, 1); ++__first, ++__cur)",
    "loop_unexpected": "for ( ; __builtin_expect(__first != __last, 0); ++__first, ++__cur)",
    "end_expected": "for ( ; !__builtin_expect(__first == __last, 1); ++__first, ++__cur)",
    "end_unexpected": "for ( ; !__builtin_expect(__first == __last, 0); ++__first, ++__cur)",
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
    address="0x0806a840",
    size=149,
    source="vuc/src/SymbolTableUnit.cpp",
)
config = OUT / "manifest.json"
config.write_text(json.dumps(manifest, indent=2) + "\n")
rows = []
for label, replacement in forms.items():
    include_dir = OUT / label
    path = include_dir / "bits/stl_uninitialized.h"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(original_header.replace(needle, replacement, 1))
    for snap in ("jan25", "dec06"):
        row = {"label": label, "compiler": snap}
        object_path = OUT / (label + "-" + snap + ".o")
        cmd = [
            "/tmp/vuc-binutils215/g++-" + snap + "-as215",
            "-O2", "-frename-registers", "-falign-jumps=1",
            "-I", str(include_dir), "-c",
            str(VUC / "src/SymbolTableUnit.cpp"), "-o", str(object_path),
        ]
        comp = subprocess.run(cmd, capture_output=True, text=True)
        if comp.returncode:
            row["error"] = comp.stderr[-380:]
        else:
            nm = subprocess.run(
                ["nm", "-S", "--defined-only", str(object_path)],
                capture_output=True, text=True, check=True,
            )
            sym = next(
                (line.split() for line in nm.stdout.splitlines()
                 if line.endswith(manifest["function"])),
                None,
            )
            row["generated_length"] = int(sym[1], 16) if sym else None
            try:
                s = probe.run(object_path, config)
                row.update(
                    exact=s["exact"], matched=s["exact_bytes"],
                    relocations=s["relocations"],
                    offsets=s["different_offsets"],
                )
            except ValueError as exc:
                row["score_error"] = str(exc)
            for name, filename in (
                ("find", "symboltable_find_probe.json"),
                ("begin", "symbol_begin_scope_probe.json"),
                ("end", "symboltable_end_exact_probe.json"),
                ("findcopy", "hash_symbol_findcopy_probe.json"),
                ("insert", "scope_vector_insert_partial_oct15i.json"),
            ):
                try:
                    s = probe.run(object_path, VUC / "config" / filename)
                    row[name] = (s["exact_bytes"], s["length"])
                except ValueError as exc:
                    row[name] = str(exc)[:75]
        rows.append(row)
        print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

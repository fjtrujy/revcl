#!/usr/bin/env python3
"""Check if legitimate STL explicit-instantiation placement changes VUC EH labels.

Normal STL weak template instantiation may be emitted by the first referencing
source unit; its actual point of instantiation can be distinct from lexical
function order. Keep the real SymbolTable methods and exact neighbors intact.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-scope-instantiation-placement-20261120")
OUT.mkdir(parents=True, exist_ok=True)
source = (VUC / "src/SymbolTableUnit.cpp").read_text()
for header in ("MLGameString.h", "MLHashTable.h", "MLHashFunGameString.h"):
    source = source.replace(
        '#include "' + header + '"',
        '#include "' + str(VUC / "src" / header) + '"',
    )
instantiation = """
#include <bits/stl_uninitialized.h>
typedef std::vector<SymbolScope> ScopeVector;
typedef ScopeVector::iterator ScopeIter;
template ScopeIter std::__uninitialized_copy_aux<ScopeIter, ScopeIter>(
    ScopeIter, ScopeIter, ScopeIter, __false_type);

"""
locations = {
    "implicit": None,
    "before_symboltable_class": "class SymbolTable {",
    "before_enter": "void SymbolTable::Enter(",
    "before_find": "SymbolTableEntry *SymbolTable::Find(",
    "before_begin": "void SymbolTable::BeginScope()",
    "before_end": "void SymbolTable::EndScope()",
    "before_findcopy": "// The original SymbolTable.cpp weakly instantiates",
    "after_findcopy": "",
}
spec = importlib.util.spec_from_file_location(
    "vuc_match_probe", VUC / "scripts/match_probe.py"
)
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)
reference = json.loads(
    (VUC / "config/hash_scope_uninitialized_probe.json").read_text()
)
reference.update(
    function=(
        "_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iterator"
        "IP11MLHashTableI12MLGameStringP16SymbolTableEntry"
        "9MLHashFunIS3_ELi513EESt6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type"
    ),
    address="0x0806a840",
    size=149,
    source="vuc/src/SymbolTableUnit.cpp",
)
config = OUT / "scope-copy.json"
config.write_text(json.dumps(reference, indent=2) + "\n")
rows = []
for label, before in locations.items():
    if before is None:
        code = source
    elif before == "":
        code = source + "\n" + instantiation
    else:
        assert before in source
        code = source.replace(before, instantiation + before, 1)
    cpp = OUT / (label + ".cpp")
    cpp.write_text(code)
    for snap in ("jan25", "dec06"):
        obj = OUT / (label + "-" + snap + ".o")
        p = subprocess.run(
            [
                "/tmp/vuc-binutils215/g++-" + snap + "-as215",
                "-O2", "-frename-registers", "-falign-jumps=1",
                "-c", str(cpp), "-o", str(obj),
            ],
            capture_output=True,
            text=True,
        )
        row = {"case": label, "compiler": snap}
        if p.returncode:
            row["error"] = p.stderr[-400:]
        else:
            nm = subprocess.run(
                ["nm", "-S", "--defined-only", str(obj)],
                capture_output=True, text=True, check=True,
            )
            target = [
                entry.split() for entry in nm.stdout.splitlines()
                if entry.endswith(reference["function"])
            ]
            if target:
                row["length"] = int(target[0][1], 16)
            try:
                result = checker.run(obj, config)
                row.update(
                    exact=result["exact"],
                    matched=result["exact_bytes"],
                    relocations=result["relocations"],
                )
            except ValueError as exc:
                row["score_error"] = str(exc)
            for name, cfg in (
                ("find", "symboltable_find_probe.json"),
                ("begin", "symbol_begin_scope_probe.json"),
                ("end", "symboltable_end_exact_probe.json"),
                ("findcopy", "hash_symbol_findcopy_probe.json"),
            ):
                try:
                    score = checker.run(obj, VUC / "config" / cfg)
                    row[name] = score["exact"]
                except ValueError as exc:
                    row[name] = "REJECT: " + str(exc)[:75]
        print(json.dumps(row, sort_keys=True), flush=True)
        rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

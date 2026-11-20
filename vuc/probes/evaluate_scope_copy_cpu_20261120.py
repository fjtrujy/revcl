#!/usr/bin/env python3
"""Check historical i386 tuning as a cause of VUC Scope EH-pad placement.

Only the real SymbolTable C++03 source unit is compiled.  Original-target
relocations come from the independently checked scope-copy manifest; all
length, relocation and original-offset scores are independently checked.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-scope-cpu-context-20261120")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location(
    "vuc_match_probe", VUC / "scripts/match_probe.py"
)
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)
base = json.loads((VUC / "config/hash_scope_uninitialized_probe.json").read_text())
base["function"] = (
    "_ZSt24__uninitialized_copy_auxIN9__gnu_cxx17__normal_iterator"
    "IP11MLHashTableI12MLGameStringP16SymbolTableEntry"
    "9MLHashFunIS3_ELi513EESt6vectorIS8_SaIS8_EEEESD_ET0_T_SF_SE_12__false_type"
)
base["address"] = "0x0806a840"
base["size"] = 149
base["source"] = "vuc/src/SymbolTableUnit.cpp"
config = OUT / "scope-copy.json"
config.write_text(json.dumps(base, indent=2) + "\n")

symbol = base["function"]
neighbors = {
    "find": "symboltable_find_probe.json",
    "begin": "symbol_begin_scope_probe.json",
    "end": "symboltable_end_exact_probe.json",
    "findcopy": "hash_symbol_findcopy_probe.json",
    "insert": "scope_vector_insert_partial_oct15i.json",
}
architectures = (
    ("default", ()),
    ("mcpu-i386", ("-mcpu=i386",)),
    ("mcpu-i486", ("-mcpu=i486",)),
    ("mcpu-i586", ("-mcpu=i586",)),
    ("mcpu-i686", ("-mcpu=i686",)),
    ("mcpu-pentiumpro", ("-mcpu=pentiumpro",)),
    ("mcpu-pentium3", ("-mcpu=pentium3",)),
    ("march-i386", ("-march=i386",)),
    ("march-i486", ("-march=i486",)),
    ("march-i586", ("-march=i586",)),
    ("march-i686", ("-march=i686",)),
    ("march-pentiumpro", ("-march=pentiumpro",)),
    ("march-pentium3", ("-march=pentium3",)),
)
rows = []
for tag, cpu_flags in architectures:
    for snapshot in ("jan25", "dec06"):
        row = {"case": tag, "compiler": snapshot}
        obj = OUT / (tag + "-" + snapshot + ".o")
        command = [
            "/tmp/vuc-binutils215/g++-" + snapshot + "-as215",
            "-O2", "-frename-registers", "-falign-jumps=1",
            *cpu_flags, "-c", str(VUC / "src/SymbolTableUnit.cpp"),
            "-o", str(obj),
        ]
        comp = subprocess.run(command, capture_output=True, text=True)
        if comp.returncode:
            row["error"] = comp.stderr[-400:]
        else:
            nm = subprocess.run(
                ["nm", "-S", "--defined-only", str(obj)],
                capture_output=True, text=True, check=True,
            )
            for line in nm.stdout.splitlines():
                words = line.split()
                if len(words) >= 4 and words[-1] == symbol:
                    row["generated_length"] = int(words[1], 16)
                    break
            try:
                result = checker.run(obj, config)
                row.update(
                    matched=result["exact_bytes"],
                    relocations=result["relocations"],
                    differences=result["different_offsets"],
                    exact=result["exact"],
                )
            except ValueError as exc:
                row["score_error"] = str(exc)
            if row.get("generated_length") == 149:
                for key, file in neighbors.items():
                    try:
                        score = checker.run(obj, VUC / "config" / file)
                        row[key] = score["exact_bytes"]
                    except ValueError as exc:
                        row[key] = "REJECT: " + str(exc)[:90]
        print(json.dumps(row, sort_keys=True), flush=True)
        rows.append(row)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

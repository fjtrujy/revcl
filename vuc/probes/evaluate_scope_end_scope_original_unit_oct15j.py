#!/usr/bin/env python3
"""Test original SymbolTable.cpp EndScope after the accepted BeginScope.

The original EndScope ends the vector's last scope via a direct clear().
In a proper std::vector<Scope> owner this is naturally spelled pop_back().
Test the genuine method with its predecessors and associated vector insertion,
not with synthetic peephole/register forcing functions.
"""
from pathlib import Path
import importlib.util
import subprocess
import json

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/scope-with-endscope-oct15j")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
unit = (VUC / "src/SymbolTableUnit.cpp").read_text()
for include in ("MLGameString.h", "MLHashTable.h", "MLHashFunGameString.h"):
    unit = unit.replace('#include "' + include + '"',
                        '#include "' + str(VUC / "src" / include) + '"')
assert "    void EndScope();" in unit
forms = {
    "original-no-endscope": unit,
    "popback": unit + "\nvoid SymbolTable::EndScope() { scopes_.pop_back(); }\n",
    "popback-if-not-empty": unit + """\nvoid SymbolTable::EndScope() {
        if (!scopes_.empty()) scopes_.pop_back();
    }
    """,
    "last-clear-popback": unit + """\nvoid SymbolTable::EndScope() {
        scopes_.back().clear();
        scopes_.pop_back();
    }
    """,
}
configs = {"insert": "scope_vector_insert_partial_oct15i.json",
           "end": "symboltable_end_exact_probe.json",
           "begin": "symbol_begin_scope_probe.json",
           "find": "symboltable_find_probe.json"}
results = []
for form, code in forms.items():
    cpp = OUT / (form + ".cpp")
    cpp.write_text(code)
    for snap in ("jan25", "dec06"):
        obj = OUT / (form + "-" + snap + ".o")
        flags = ["-O2", "-frename-registers", "-falign-jumps=1"]
        cp = subprocess.run(["/tmp/vuc-binutils215/g++-" + snap + "-as215",
                             *flags, "-c", str(cpp), "-o", str(obj)],
                            capture_output=True, text=True)
        row = {"form": form, "snapshot": snap, "functions": {}}
        if cp.returncode:
            row["error"] = cp.stderr[-300:]
        else:
            for target, filename in configs.items():
                try:
                    s = m.run(obj, VUC / "config" / filename)
                    row["functions"][target] = {"bytes": s["exact_bytes"],
                                                 "length": s["length"],
                                                 "relocations": s["relocations"],
                                                 "diff": s["different_offsets"]}
                except ValueError as e:
                    row["functions"][target] = {"error": str(e)[:170]}
        results.append(row)
        print(form, snap,
              {k: (v.get("bytes"), v.get("length")) for k, v in row["functions"].items()},
              row.get("error", "")[:160], flush=True)
(OUT / "results.json").write_text(json.dumps(results, indent=2) + "\n")

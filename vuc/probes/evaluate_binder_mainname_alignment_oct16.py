#!/usr/bin/env python3
"""Test actual C++03 Binder main-name lifetime with GCC label boundaries.

Only original-length source with all authentic relocations would be eligible
for EXPERIMENTAL classification. Other lengths are saved negative evidence.
"""
from pathlib import Path
import subprocess

R = Path(__file__).resolve().parents[1]
O = Path("/tmp/vuc-binder-mainname-alignment-oct16")
O.mkdir(exist_ok=True)
original = (R / "probes/BinderBindASTModuleFreshOct15s.cpp").read_text()
for header in ("MLGameString.h", "MLHashFunGameString.h",
               "MLHashTable.h", "ASTNodeList.h"):
    original = original.replace(
        '#include "../src/' + header + '"',
        '#include "' + str(R / "src" / header) + '"')
anchor = ('    MLGameString mainName("main",false);\n'
          '    if (!table.Find(mainName,false)) exportModule=true;')
assert anchor in original
forms = {
    "baseline": anchor,
    "eq-null": '    MLGameString mainName("main",false);\n    if(table.Find(mainName,false)==0) exportModule=true;',
    "local-entry": '    MLGameString mainName("main",false);\n    SymbolTableEntry *entry=table.Find(mainName,false); if(!entry) exportModule=true;',
    "local-missing": '    MLGameString mainName("main",false);\n    bool missing=table.Find(mainName,false)==0; if(missing) exportModule=true;',
    "local-found": '    MLGameString mainName("main",false);\n    bool found=table.Find(mainName,false)!=0; if(!found) exportModule=true;',
    "inverted-if": '    MLGameString mainName("main",false);\n    if(table.Find(mainName,false)) {} else exportModule=true;',
    "if-block": '    MLGameString mainName("main",false);\n    if(!table.Find(mainName,false)) { exportModule=true; }',
    "assign-true": '    MLGameString mainName("main",false);\n    if(!table.Find(mainName,false)) exportModule=1;',
    "main-ref": '    const MLGameString mainName("main",false);\n    if(!table.Find(mainName,false)) exportModule=true;',
    "name-inner-scope": '    { MLGameString mainName("main",false);\n      if(!table.Find(mainName,false)) exportModule=true; }',
    "name-local-args": '    MLGameString mainName("main",false);\n    SymbolTable &owner=table; if(!owner.Find(mainName,false)) exportModule=true;',
    "name-extra-local": '    MLGameString mainName("main",false);\n    bool withinCurrentScope=false; if(!table.Find(mainName,withinCurrentScope)) exportModule=true;',
}
opts = ["-O2", "-frename-registers", "-falign-jumps=8",
        "-falign-labels=4", "-falign-loops=1"]
for name, body in forms.items():
    cpp = O / (name + ".cpp")
    cpp.write_text(original.replace(anchor, body))
    obj = O / (name + ".o")
    p = subprocess.run(
        ["/tmp/vuc-binutils215/g++-jan25-as215",
         *opts, "-c", str(cpp), "-o", str(obj)],
        capture_output=True, text=True)
    if p.returncode:
        print("FAIL", name, p.stderr[-150:], flush=True)
        continue
    n = subprocess.run(["nm", "-S", str(obj)], capture_output=True, text=True)
    sizes = [int(line.split()[1], 16) for line in n.stdout.splitlines()
             if line.endswith(" T _Z11Binder_BindP9ASTModule")]
    if sizes:
        print("RESULT", name, sizes[0],
              "ORIGINAL_LENGTH" if sizes[0] == 552 else "", flush=True)

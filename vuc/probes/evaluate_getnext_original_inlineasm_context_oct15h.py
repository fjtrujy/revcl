#!/usr/bin/env python3
"""Recover GetNextWord GCC register state from original InlineAsm.cpp neighbors.

The original has BindInlineAsm and GenInlineAsm before GetNextWord. A real
FlexLexer predecessor recovered a full exact function. This probe tests
analogous ordinary C++03 source-unit prefixes from the existing provisional
InlineAsm reconstruction, not dummy methods or register-forcing assembly.
"""
from pathlib import Path
import json
import importlib.util
import itertools
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/getnext-original-inlineasm-context-oct15h")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg = VUC / "config/get_next_word_partial_oct15e_probe.json"
chosen = (VUC / "probes/GetNextWordNestedRangePartialOct15f.cpp").read_text()
intro = chosen[:chosen.index("const char *GetNextWord(")]
method = chosen[chosen.index("const char *GetNextWord("):]
context = (VUC / "probes/inline_asm_source_context_oct13.cpp").read_text()
header = context[:context.index("void BindInlineAsm(")]
header = header.replace('#include "../src/MLGameString.h"',
                        '#include "' + str(VUC / "src/MLGameString.h") + '"')
prefix = context[context.index("void BindInlineAsm("):context.index("void GenInlineAsm(")]
next_fn = context[context.index("void GenInlineAsm("):context.index("const char *GetNextWord(", context.index("void GenInlineAsm("))]
header = header.replace("extern unsigned currentPosition;", "extern int currentPosition;")
header = header.replace("extern unsigned wordPosition;", "extern int wordPosition;")
variants = {
    "isolated": intro + method,
    "bind-before": header + prefix + "\n" + method,
    "gen-before": header + next_fn + "\n" + method,
    "bind-gen-before": header + prefix + "\n" + next_fn + "\n" + method,
    "gen-bind-before": header + next_fn + "\n" + prefix + "\n" + method,
}
flags = {
    "base": ["-O2", "-frename-registers", "-falign-jumps=16", "-falign-labels=1", "-falign-loops=2"],
    "j8": ["-O2", "-frename-registers", "-falign-jumps=8", "-falign-labels=1", "-falign-loops=2"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4", "-falign-labels=1", "-falign-loops=2"],
    "plain": ["-O2", "-falign-jumps=16", "-falign-labels=1", "-falign-loops=2"],
}
records = []
best = None
for (variant, source), (flagkey, opts) in itertools.product(variants.items(), flags.items()):
    tag = f"{variant}-{flagkey}"
    cpp = OUT / (tag + ".cpp")
    obj = OUT / (tag + ".o")
    cpp.write_text(source)
    p = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts,
                        "-c", str(cpp), "-o", str(obj)], text=True, capture_output=True)
    if p.returncode:
        records.append({"tag": tag, "error": p.stderr[-230:]})
        continue
    try:
        score = matcher.run(obj, cfg)
    except ValueError as e:
        records.append({"tag": tag, "error": str(e)})
        continue
    row = {"tag": tag, "length": score["length"], "match": score["exact_bytes"],
           "relocations": score["relocations"], "different_offsets": score["different_offsets"]}
    records.append(row)
    if (row["length"], row["relocations"]) != (135, 5):
        continue
    if best is None or row["match"] > best["match"]:
        best = row
        (OUT / "best.cpp").write_text(source)
        (OUT / "best.o").write_bytes(obj.read_bytes())
        (OUT / "best.flags").write_text(" ".join(opts) + "\n")
        print("BEST", json.dumps(row), flush=True)
    if row["match"] == 135:
        break
(OUT / "results.json").write_text(json.dumps(records, indent=2) + "\n")
print(json.dumps({"attempted": len(records), "eligible": sum(x.get("length") == 135 and x.get("relocations") == 5 for x in records), "best": best}, indent=2))

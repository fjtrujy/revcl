#!/usr/bin/env python3
"""GetNextWord 50/135: real signed leading-character and branch forms.

The original compares signed DL against 10 and 9 (jg/jl), whereas the
50-byte candidate uses unsigned ja/jbe and compares against 8. Test the
historically plausible inefficient spelling that preserves the original
branch order, without assembly, extra padding or hard register binding.
"""
from pathlib import Path
import collections
import importlib.util
import itertools
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/getnext-signed-leading-oct15h")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("m", VUC / "scripts/match_probe.py")
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
cfg = VUC / "config/get_next_word_partial_oct15e_probe.json"
base = (VUC / "probes/GetNextWordNestedRangePartialOct15f.cpp").read_text()
start = base.index(" const unsigned char c = *input;")
end = base.index(" ++position; ++input; currentPosition=position;", start)
types = {
    "char": "char",
    "signedchar": "signed char",
    "unsignedchar": "unsigned char",
    "int": "int",
}
branches = {
    "baseline": """if (c <= 10) { if (c < 9) break; }
 else if (c != 32) break;""",
    "hi-first": """if (c > 10) { if (c != 32) break; }
 else if (c < 9) break;""",
    "two-independent": """if (c > 10 && c != 32) break;
 if (c < 9) break;""",
    "hi-eq": """if (c > 10) { if (c == 32) {} else break; }
 else if (c < 9) break;""",
    "explicit-consume": """if (c > 10) {
  if (c == 32) goto consume;
  break;
 }
 if (c < 9) break;
 consume:;""",
    "explicit-stop": """if (c > 10) goto checkspace;
 if (c < 9) break;
 goto consume;
 checkspace:
 if (c != 32) break;
 consume:;""",
    "nested-low": """if (c > 10) { if (c == 32) {} else break; }
 else { if (c < 9) break; }""",
    "negative-low": """if (c > 10) { if (c != 32) break; }
 else { if (!(c >= 9)) break; }""",
    "explicit-low-positive": """if (c > 10) { if (c != 32) break; }
 else if (c >= 9) {} else break;""",
    "zero-extra": """if (c == 0) break;
 if (c > 10) { if (c != 32) break; }
 else if (c < 9) break;""",
    "space-first": """if (c == 32) goto consume;
 if (c < 9 || c > 10) break;
 consume:;""",
}
flags = {
    "selected": ["-O2", "-frename-registers", "-falign-jumps=16", "-falign-labels=1", "-falign-loops=2"],
    "j8": ["-O2", "-frename-registers", "-falign-jumps=8", "-falign-labels=1", "-falign-loops=2"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4", "-falign-labels=1", "-falign-loops=2"],
    "no-ren": ["-O2", "-falign-jumps=16", "-falign-labels=1", "-falign-loops=2"],
}
results = []
counts = collections.Counter()
best = None
for (typekey, typename), (branchkey, predicate), (flagkey, options) in itertools.product(
        types.items(), branches.items(), flags.items()):
    tag = f"{typekey}-{branchkey}-{flagkey}"
    source = base[:start] + f" {typename} c = *input;\n " + predicate + "\n" + base[end:]
    cpp = OUT / (tag + ".cpp")
    obj = OUT / (tag + ".o")
    cpp.write_text(source)
    p = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *options,
                        "-c", str(cpp), "-o", str(obj)],
                       text=True, capture_output=True)
    if p.returncode:
        results.append({"tag": tag, "error": p.stderr[-180:]})
        continue
    try:
        score = m.run(obj, cfg)
    except ValueError as e:
        results.append({"tag": tag, "error": str(e)})
        continue
    row = {
        "tag": tag, "length": score["length"],
        "match": score["exact_bytes"], "relocations": score["relocations"],
        "different_offsets": score["different_offsets"],
        "prefix": next(iter(score["different_offsets"]), score["length"]),
    }
    results.append(row)
    counts[row["length"]] += 1
    if row["length"] != 135 or row["relocations"] != 5:
        continue
    if best is None or (row["match"], row["prefix"]) > (best["match"], best["prefix"]):
        best = row
        (OUT / "best.cpp").write_text(source)
        (OUT / "best.o").write_bytes(obj.read_bytes())
        (OUT / "best.flags").write_text(" ".join(options) + "\n")
        print("BEST", json.dumps(row), flush=True)
    if row["match"] == 135:
        break
(OUT / "results.json").write_text(json.dumps(results, indent=2) + "\n")
print(json.dumps({"attempted": len(results), "lengths": dict(counts),
                  "eligible": sum(r.get("length") == 135 and r.get("relocations") == 5 for r in results),
                  "best": best}, indent=2))

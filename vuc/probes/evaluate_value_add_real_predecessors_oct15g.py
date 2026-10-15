#!/usr/bin/env python3
"""Historical GCC predecessor-lifetime probes for VUC Value::operator+.

The real ELF order preceding addition is Set(ASTExpr), Set(ASTConstValExpr),
Set(ASTLValExpr), then operator+. Only authentic, reconstructed methods are
injected as predecessors; no assembly, fake layout or register binding.
"""
from pathlib import Path
import itertools
import json
import subprocess
import importlib.util

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
OUT = Path("/tmp/vuc-binutils215/value-plus-real-prefix-oct15g")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg = VUC / "config/value_addition_partial_oct14_probe.json"
source = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
decl = "struct Value {"
method = "Value Value::operator+(const Value &other) const"
assert decl in source and method in source

preds = {
    "lval": """void Value::Set(const ASTLValExpr *) {}""",
    "expr": """void Value::Set(const ASTExpr *expr)
{
    if (expr->kind == 2)
        Set((const ASTConstValExpr *)expr);
    else if (expr->kind == 3)
        Set((const ASTLValExpr *)expr);
}""",
}
header = """struct ASTExpr { unsigned field0, field1, kind; };
struct ASTLValExpr;
struct ASTConstValExpr;
"""
decls = """    void Set(const ASTExpr *);
    void Set(const ASTLValExpr *);
    void Set(const ASTConstValExpr *);
"""
flags = {
    "j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "j2": ["-O2", "-frename-registers", "-falign-jumps=2"],
    "j3": ["-O2", "-frename-registers", "-falign-jumps=3"],
    "j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "plain1": ["-O2", "-falign-jumps=1"],
    "plain4": ["-O2", "-falign-jumps=4"],
    "no-cross": ["-O2", "-frename-registers", "-falign-jumps=1", "-fno-crossjumping"],
}
pred_seqs = [(), ("lval",), ("expr",), ("expr", "lval"), ("lval", "expr")]
variants = {"lane3120": source}
ascending = source.replace(
    """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];""",
    """            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];"""
)
assert ascending != source
variants["ascending"] = ascending
rows = []
best = None
for sequence, (shape, template), (label, options) in itertools.product(pred_seqs, variants.items(), flags.items()):
    tag = "-".join(sequence) if sequence else "none"
    key = f"{tag}-{shape}-{label}"
    prefix = ("\n".join(preds[s] for s in sequence) + "\n") if sequence else ""
    text = template
    if prefix:
        text = header + text
        text = text.replace("    Value operator+(const Value &other) const;",
                            "    Value operator+(const Value &other) const;\n" + decls, 1)
        text = text.replace(method, prefix + method, 1)
    cpp = OUT / (key + ".cpp")
    obj = OUT / (key + ".o")
    cpp.write_text(text)
    result = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *options,
                             "-c", str(cpp), "-o", str(obj)],
                            capture_output=True, text=True)
    if result.returncode:
        rows.append({"key": key, "error": result.stderr[-160:]})
        continue
    try:
        checked = matcher.run(obj, cfg)
    except ValueError as e:
        rows.append({"key": key, "error": str(e)})
        continue
    row = {"key": key, "length": checked["length"], "matched": checked["exact_bytes"],
           "relocations": checked["relocations"], "different": checked["different_offsets"]}
    rows.append(row)
    if row["length"] != 219 or row["relocations"] != 1:
        continue
    if best is None or row["matched"] > best["matched"]:
        best = row
        (OUT / "best.cpp").write_text(text)
        (OUT / "best.flags").write_text(" ".join(options) + "\n")
        (OUT / "best.o").write_bytes(obj.read_bytes())
        print("BEST", json.dumps(best), flush=True)
    if row["matched"] == 219:
        break
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")
print(json.dumps({"tested": len(rows), "best": best,
                  "valid": sum(r.get("length") == 219 and r.get("relocations") == 1 for r in rows)}, indent=2))

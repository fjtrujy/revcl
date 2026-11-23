#!/usr/bin/env python3
"""Test VCL-inspired static-inline arithmetic helpers and TU splits for Value+.

The accepted 203/219 original-sized experimental Value::operator+ uses
authentic 3,1,2,0 vector-lane C++ semantics. Try genuine helper inlining and
source-owner boundaries; score strict original-length, relocation-verified
GCC 3.3.3 objects only, with no byte patches, ASM or artificial padding.
"""
from pathlib import Path
import importlib.util
import json
import subprocess

VUC = Path(__file__).resolve().parents[1]
OUT = Path("/tmp/vuc-value-add-inline-tus-20261123")
OUT.mkdir(exist_ok=True)
source = (VUC / "probes/ValueAdditionLaneOrderOct15.cpp").read_text()
plus = source.index("Value Value::operator+(const Value &other) const")
minus = source.index("Value Value::operator-(const Value &other) const")
multiply = source.index("Value Value::operator*(const Value &other) const")
head = source[:plus]
add = source[plus:minus]
sub = source[minus:multiply]
mul = source[multiply:]
lane_block = """            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];"""
assert lane_block in add
helpers = {
    "baseline": ("", add),
    "inline-all-lanes": (
        """static inline void addVector4(Value &dst, const Value &src) {
    dst.values[3] += src.values[3];
    dst.values[1] += src.values[1];
    dst.values[2] += src.values[2];
    dst.values[0] += src.values[0];
}
""",
        add.replace(lane_block, "            addVector4(result, temporary);")),
    "inline-single-lane": (
        """static inline void addLane(Value &dst, const Value &src, int index) {
    dst.values[index] += src.values[index];
}
""",
        add.replace(lane_block, """            addLane(result, temporary, 3);
            addLane(result, temporary, 1);
            addLane(result, temporary, 2);
            addLane(result, temporary, 0);""")),
    "inline-float-lane": (
        """static inline void addScalar(float &dst, float src) {
    dst += src;
}
""",
        add.replace(lane_block, """            addScalar(result.values[3], temporary.values[3]);
            addScalar(result.values[1], temporary.values[1]);
            addScalar(result.values[2], temporary.values[2]);
            addScalar(result.values[0], temporary.values[0]);""")),
    "inline-type-convert": (
        """static inline bool convertAddOperands(const Value &a, const Value &b,
                                              Value &dst, Value &tmp) {
    return Value::TypeConvert(a,b,dst,tmp);
}
""",
        add.replace("TypeConvert(*this, other, result, temporary)",
                    "convertAddOperands(*this, other, result, temporary)")),
    "inline-int-add": (
        """static inline void addInteger(Value &dst, const Value &src) {
    dst.bits += src.bits;
}
""",
        add.replace("case 1: result.bits += temporary.bits; break;",
                    "case 1: addInteger(result, temporary); break;")),
    "inline-scalar-float": (
        """static inline void addOneFloat(Value &dst, const Value &src) {
    dst.values[0] += src.values[0];
}
""",
        add.replace("case 3: result.values[0] += temporary.values[0]; break;",
                    "case 3: addOneFloat(result, temporary); break;")),
}
spec = importlib.util.spec_from_file_location("match", VUC / "scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
cfg = VUC / "config/value_addition_partial_oct14_probe.json"
rows = []
for name, (helper, original_add) in helpers.items():
    for owners in ("add-only", "add-sub-mul", "sub-add-mul"):
        body = original_add if owners == "add-only" else (
            original_add + sub + mul if owners == "add-sub-mul" else sub + original_add + mul)
        text = head + helper + "\n" + body
        filename = OUT / (name + "-" + owners + ".cpp")
        filename.write_text(text)
        for compiler in ("jan25", "dec06"):
            output = OUT / (name + "-" + owners + "-" + compiler + ".o")
            cmd = ["/tmp/vuc-binutils215/g++-" + compiler + "-as215",
                   "-O2", "-frename-registers", "-falign-jumps=1",
                   "-c", str(filename), "-o", str(output)]
            p = subprocess.run(cmd, capture_output=True, text=True)
            row = {"case": name, "owners": owners, "compiler": compiler}
            if p.returncode:
                row["compile_failure"] = p.stderr[-350:]
            else:
                try:
                    s = matcher.run(output, cfg)
                    row.update(size=s["length"], matched=s["exact_bytes"],
                               exact=s["exact"], differences=s["different_offsets"],
                               relocations=s["relocations"])
                except (ValueError, KeyError) as ex:
                    row["unverified"] = str(ex)
            rows.append(row)
            print(json.dumps(row, sort_keys=True), flush=True)
(OUT / "results.json").write_text(json.dumps(rows, indent=2) + "\n")

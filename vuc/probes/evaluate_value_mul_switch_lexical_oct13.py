#!/usr/bin/env python3
"""Test genuine lexical switch-arm ordering for Value multiplication under GCC 3.3.3.

VCL bbe5056 demonstrated that moving intact arms, without changing their
semantics, can alter peephole2 scratch selection. Keep all three VUC
multiplication arms and authentic relocations, testing isolated and real
neighboring Value source-unit contexts independently.
"""
from itertools import permutations
from pathlib import Path
import importlib.util
import subprocess
R = Path(__file__).resolve().parents[2]
T = Path("/tmp/vuc-binutils215/value-mul-lexical-oct13")
T.mkdir(parents=True, exist_ok=True)
sp = importlib.util.spec_from_file_location("m", R / "vuc/scripts/match_probe.py")
m = importlib.util.module_from_spec(sp)
sp.loader.exec_module(m)
raws = {
    "isolated": (R / "vuc/probes/value_ml.cpp").read_text(),
    "neighbors": (R / "vuc/src/Value.cpp").read_text(),
}
raws["neighbors"] = raws["neighbors"].replace(
    "    Value();", "    Value() : type(0), a(0), b(0), flag0(0), flag1(0), flag2(0), flag3(0) {}"
).replace("Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}", "")
flags = {
    "ren-j1": ["-O2", "-frename-registers", "-falign-jumps=1"],
    "ren-j4": ["-O2", "-frename-registers", "-falign-jumps=4"],
    "plain-j1": ["-O2", "-falign-jumps=1"],
    "ren-no-gcse": ["-O2", "-frename-registers", "-falign-jumps=1", "-fno-gcse"],
}
rank = []
for context, code in raws.items():
    start = code.index("Value Value::operator*(")
    switch = code.index("switch (result.type)", start)
    close = code.index("\n        }\n", switch)
    begin = code.index("\n        case 1:", switch)
    blocks = code[begin:close].split("\n        case ")
    assert len(blocks) == 4, blocks
    arms = {int(arm.split(":", 1)[0]): arm for arm in blocks[1:]}
    assert set(arms) == {1, 3, 4}
    for order in permutations((1, 3, 4)):
        modified = code[:begin] + "".join("\n        case " + arms[n] for n in order) + code[close:]
        src = T / (context + "-" + "".join(map(str, order)) + ".cpp")
        src.write_text(modified)
        for tag, opts in flags.items():
            obj = T / (src.stem + "-" + tag + ".o")
            proc = subprocess.run(["/tmp/vuc-binutils215/g++-jan25-as215", *opts, "-c", str(src), "-o", str(obj)], capture_output=True, text=True)
            if proc.returncode:
                print("COMPILE", context, order, tag, proc.stderr[-180:], flush=True)
                continue
            try:
                r = m.run(obj, R / "vuc/config/value_ml_probe.json")
                rank.append((r["exact_bytes"], context, order, tag, r["length"]))
                if r["exact"]:
                    print("BYTE_EXACT", context, order, tag, src, flush=True)
                    raise SystemExit(0)
            except ValueError as e:
                rank.append((0, context, order, tag, str(e)[:85]))
        print("checked", context, order, flush=True)
print("TOP")
for row in sorted(rank, reverse=True)[:26]:
    print(row)

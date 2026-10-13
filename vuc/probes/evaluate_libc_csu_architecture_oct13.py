#!/usr/bin/env python3
"""Check historical i386 vs i486 GCC codegen for glibc's PIC initializer.

GCC 3.3 changes callee-saved register prologues for -march=i386/i486.
Check genuine alignment settings in the same libc startup C translation unit;
also score already-accepted __libc_csu_fini to detect regressions.
"""
from itertools import product
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/csu-i386-arch-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
source = ROOT / "vuc/src/LibcCsu.c"
init_cfg = ROOT / "vuc/config/libc_csu_init_pic_probe.json"
fini_cfg = ROOT / "vuc/config/libc_csu_fini_pic_probe.json"
results = []
for arch, loop, jump, rename in product(
    ("i386", "i486", "i586"),
    ("1", "4", "8", "16"),
    ("1", "4", "8", "16"),
    (False, True),
):
    flags = ["-xc", "-O2", "-fPIC", "-march=" + arch,
             "-falign-loops=" + loop, "-falign-jumps=" + jump]
    if rename:
        flags += ["-frename-registers"]
    key = "%s-l%s-j%s-r%s" % (arch, loop, jump, int(rename))
    obj = SCRATCH / (key + ".o")
    build = subprocess.run(
        ["/tmp/vuc-binutils215/g++-jan25-as215", *flags, "-c", str(source), "-o", str(obj)],
        capture_output=True, text=True)
    if build.returncode:
        print("COMPILE_FAIL", key, build.stderr[-100:], flush=True)
        continue
    sizes = {row.split()[-1]: int(row.split()[1], 16)
             for row in subprocess.check_output(["nm", "-S", str(obj)], text=True).splitlines()
             if len(row.split()) == 4}
    n = sizes.get("__libc_csu_init", -1)
    scores = []
    if n == 84:
        for cfg in (init_cfg, fini_cfg):
            try:
                res = matcher.run(obj, cfg)
                scores.append((res["exact_bytes"], res["length"], res["exact"]))
            except ValueError as exc:
                scores.append(("reject:" + str(exc)[:70], 0, False))
        print("LENGTH_MATCH", key, scores, flush=True)
        if scores[0][2] and scores[1][2]:
            print("FULL_STARTUP_EXACT", key, flush=True)
            raise SystemExit(0)
    results.append((scores[0][0] if scores and isinstance(scores[0][0], int) else 0,
                    -abs(84-n), n, key))
print("BEST")
for result in sorted(results, reverse=True)[:30]:
    print(result)

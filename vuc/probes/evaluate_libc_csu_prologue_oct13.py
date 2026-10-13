#!/usr/bin/env python3
"""Diagnose the historical PIC csu initializer's saved-register prologue.

These controls vary supported optimization and i386 ABI settings, not emitted
instructions. The target is the canonical glibc 2.3.2 84-byte source function.
"""
import importlib.util
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/csu-prologue-oct13")
SCRATCH.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location("matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)
sources = {
    "unit": ROOT / "vuc/src/LibcCsu.c",
    "isolated": ROOT / "vuc/probes/libc_csu_hidden.c",
}
options = {
    "o1": ["-O1", "-fPIC"],
    "o2": ["-O2", "-fPIC"],
    "o3": ["-O3", "-fPIC"],
    "os": ["-Os", "-fPIC"],
    "o2-rename": ["-O2", "-fPIC", "-frename-registers"],
    "o2-no-sched": ["-O2", "-fPIC", "-fno-schedule-insns2"],
    "o2-no-regmove": ["-O2", "-fPIC", "-fno-regmove"],
    "o2-no-reorder": ["-O2", "-fPIC", "-fno-reorder-blocks"],
    "o2-no-cse": ["-O2", "-fPIC", "-fno-cse-follow-jumps"],
    "o2-boundary2": ["-O2", "-fPIC", "-mpreferred-stack-boundary=2"],
    "o2-boundary3": ["-O2", "-fPIC", "-mpreferred-stack-boundary=3"],
    "o2-i386": ["-O2", "-fPIC", "-march=i386"],
    "o2-i486": ["-O2", "-fPIC", "-march=i486"],
    "o2-i686": ["-O2", "-fPIC", "-march=i686"],
    "o2-loop16": ["-O2", "-fPIC", "-falign-loops=16"],
    "o2-no-peephole2": ["-O2", "-fPIC", "-fno-peephole2"],
    "o2-bound2-loop16": ["-O2", "-fPIC", "-mpreferred-stack-boundary=2", "-falign-loops=16"],
}
cfg = ROOT / "vuc/config/libc_csu_init_pic_probe.json"
rank = []
for source_name, source in sources.items():
    for name, flags in options.items():
        obj = SCRATCH / (source_name + "-" + name + ".o")
        build = subprocess.run(
            ["/tmp/vuc-binutils215/g++-jan25-as215", "-xc", *flags,
             "-c", str(source), "-o", str(obj)],
            capture_output=True, text=True)
        if build.returncode:
            print("COMPILE_FAIL", source_name, name, build.stderr[-90:], flush=True)
            continue
        nm = subprocess.check_output(["nm", "-S", str(obj)], text=True)
        entry = next((x for x in nm.splitlines() if x.endswith(" __libc_csu_init")), None)
        size = int(entry.split()[1], 16) if entry else -1
        score = None
        if size == 84:
            try:
                matched = matcher.run(obj, cfg)
                score = matched["exact_bytes"]
                if matched["exact"]:
                    print("EXACT", source_name, name, str(source), flush=True)
                    raise SystemExit(0)
            except ValueError as error:
                print("REJECT", source_name, name, str(error)[:120], flush=True)
        dump = subprocess.check_output(["objdump", "-dr", "-Mintel", str(obj)], text=True)
        code = dump.split("<__libc_csu_init>:", 1)[-1].split("\n\n", 1)[0]
        first = [line.strip().split("\t")[-1] for line in code.splitlines()[:10]]
        rank.append((score or 0, size, source_name, name))
        print("CHECK", source_name, name, "size", size, "match", score,
              "prologue", " / ".join(first[:6]), flush=True)
print("RANKED")
for row in sorted(rank, reverse=True)[:25]:
    print(row)

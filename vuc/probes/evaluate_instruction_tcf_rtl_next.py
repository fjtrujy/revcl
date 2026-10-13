#!/usr/bin/env python3
"""Reproduce the real GCC 3.3.3 dead-register difference in VUC __tcf_1.

Compile genuine Instruction arena source with RTL dumps. Compare the
generated 46-byte source function (45/46 bytes matching) with the
original ELF, and inspect the original compiler's flow2 / peephole2 /
rename-registers passes. Never patch binaries or force a register.
"""
from pathlib import Path
import subprocess
import importlib.util
import re

R = Path(__file__).resolve().parents[2]
T = Path("/tmp/vuc-binutils215/instruction-rtl-reproduction-next")
T.mkdir(parents=True, exist_ok=True)
sp = importlib.util.spec_from_file_location("matcher", R / "vuc/scripts/match_probe.py")
m = importlib.util.module_from_spec(sp)
sp.loader.exec_module(m)

source = R / "vuc/probes/instruction_list_arena_unit_oct14.cpp"
config = R / "vuc/config/instruction_arena_cleanup_unit_oct14_probe.json"
for label, compiler in (
    ("jan25", "/tmp/vuc-binutils215/g++-jan25-as215"),
    ("dec06", "/tmp/vuc-binutils215/g++-dec06-as215"),
):
    out = T / label
    out.mkdir(exist_ok=True)
    obj = out / "instruction.o"
    cmd = [
        compiler, "-O2", "-frename-registers", "-fuse-cxa-atexit",
        "-da", "-c", str(source), "-o", str(obj)
    ]
    proc = subprocess.run(cmd, cwd=out, capture_output=True, text=True)
    if proc.returncode:
        print("COMPILER_ERROR", label, proc.stderr[-400:], flush=True)
        continue

    result = m.run(obj, config)
    print("MATCH", label, "%s/%s" % (result["exact_bytes"], result["length"]),
          "offsets", result["different_offsets"], flush=True)

    disasm = subprocess.check_output(
        ["objdump", "-dr", "-Mintel", str(obj)], text=True
    )
    at = disasm.index("<__tcf_1>:")
    stop = disasm.index("\n\n", at)
    last = disasm[at:stop]
    pop = re.findall(r"\bpop\s+(\w+)", last)
    print("EPILOGUE_POP_REGISTERS", label, pop[-3:], flush=True)

    for suffix in ("26.flow2", "27.peephole2", "28.rnreg", "30.sched2"):
        paths = list(out.glob("*." + suffix))
        if not paths:
            print("NO_RTL_PASS", label, suffix, flush=True)
            continue
        text = paths[0].read_text(errors="replace")
        marker = ";; Function void __tcf_1(void*)"
        if marker not in text:
            print("NO_DESTRUCTOR_IN_PASS", label, suffix, flush=True)
            continue
        text = text[text.index(marker) + len(marker):].split(";; Function ", 1)[0]
        # Restrict to the destructor's epilogue. In this GCC version
        # the unused scratch destination is a set(REG) in the pop RTL.
        epilogue = text.split("NOTE_INSN_EPILOGUE_BEG")[-1]
        destinations = re.findall(
            r"\(set \(reg:SI (\d+) (\w+)\)", epilogue
        )
        print("RTL_EPILOGUE_DESTINATIONS", label, suffix,
              destinations[:4], flush=True)

#!/usr/bin/env python3
"""Historical C/PIC glibc fini-array loop source variants, byte-exact only."""
from pathlib import Path
import importlib.util
import subprocess

R = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215")
spec = importlib.util.spec_from_file_location("vuc_match", R / "vuc/scripts/match_probe.py")
match = importlib.util.module_from_spec(spec)
spec.loader.exec_module(match)
source = (R / "vuc/probes/libc_csu_hidden.c").read_text()
start = source.index("void __libc_csu_fini(void)")
prefix = source[:start]
variants = {
    "postwhile": """unsigned count = __fini_array_end - __fini_array_start;
    while (count-- > 0)
        __fini_array_start[count]();
    _fini();""",
    "prewhile": """unsigned count = __fini_array_end - __fini_array_start;
    while (count > 0) {
        --count;
        __fini_array_start[count]();
    }
    _fini();""",
    "fordecrement": """unsigned count = __fini_array_end - __fini_array_start;
    for (; count > 0; --count)
        __fini_array_start[count - 1]();
    _fini();""",
    "forpost": """unsigned count = __fini_array_end - __fini_array_start;
    for (; count > 0;)
        __fini_array_start[--count]();
    _fini();""",
    "separatecount": """unsigned count;
    count = __fini_array_end - __fini_array_start;
    while (count-- > 0)
        __fini_array_start[count]();
    _fini();""",
    "signed": """int count = __fini_array_end - __fini_array_start;
    while (count-- > 0)
        __fini_array_start[count]();
    _fini();""",
    "int-for": """int count = __fini_array_end - __fini_array_start;
    for (int i=count-1; i>=0; --i)
        __fini_array_start[i]();
    _fini();""",
    "i-from-end": """unsigned n = __fini_array_end - __fini_array_start;
    for (unsigned i=n; i; )
        __fini_array_start[--i]();
    _fini();""",
    "i-decrement": """unsigned n = __fini_array_end - __fini_array_start;
    for (unsigned i=n; i != 0; --i)
        __fini_array_start[i-1]();
    _fini();""",
    "i-post": """unsigned n = __fini_array_end - __fini_array_start;
    for (unsigned i=n; i-- > 0;)
        __fini_array_start[i]();
    _fini();""",
    "pointer": """startup_hook *start = __fini_array_start;
    unsigned count = __fini_array_end - start;
    while (count-- > 0)
        start[count]();
    _fini();""",
    "two-pointers": """startup_hook *begin = __fini_array_start;
    startup_hook *end = __fini_array_end;
    unsigned count = end - begin;
    while (count-- > 0)
        begin[count]();
    _fini();""",
    "post-do": """unsigned count = __fini_array_end - __fini_array_start;
    if (count > 0) do {
        --count;
        __fini_array_start[count]();
    } while (count);
    _fini();""",
    "for-less": """unsigned count = __fini_array_end - __fini_array_start;
    unsigned i = 0;
    for (; i < count; ++i)
        __fini_array_start[count-i-1]();
    _fini();""",
    "do-while": """unsigned count = __fini_array_end - __fini_array_start;
    if (count) {
        do { __fini_array_start[--count](); }
        while (count);
    }
    _fini();""",
}
compiler = SCRATCH / "g++-jan25-as215"
flags = ["-xc", "-O2", "-fPIC", "-frename-registers", "-falign-jumps=1"]
ranking = []
for name, body in variants.items():
    cpp = SCRATCH / ("csu-fini-" + name + ".c")
    obj = SCRATCH / ("csu-fini-" + name + ".o")
    cpp.write_text(prefix + "void __libc_csu_fini(void)\n{\n    " + body + "\n}\n")
    cmd = [str(compiler), *flags, "-c", str(cpp), "-o", str(obj)]
    run = subprocess.run(cmd, capture_output=True, text=True)
    if run.returncode:
        ranking.append((0, name, run.stderr[-120:]))
        continue
    try:
        result = match.run(obj, R / "vuc/config/libc_csu_fini_pic_probe.json")
        ranking.append((result["exact_bytes"], name, str(result["length"]),
                        str(result["different_offsets"])))
        if result["exact"]:
            print("NEW EXACT", name, flush=True)
    except ValueError as exc:
        ranking.append((0, name, str(exc)[:110]))
for item in sorted(ranking, reverse=True):
    print(*item)

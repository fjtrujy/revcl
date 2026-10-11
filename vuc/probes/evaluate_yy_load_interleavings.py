#!/usr/bin/env python3
"""Probe source-level load/store order for FlexLexer buffer state recovery.

Only ordinary C++ assignments are emitted; every candidate is kept outside
the accepted source tree and checked against the original ELF function.
"""
import importlib.util
import itertools
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SCRATCH = Path("/tmp/vuc-binutils215/yy-load-interleavings")
COMPILER = Path("/tmp/vuc-binutils215/g++-jan25-as215")
SOURCE = ROOT / "vuc/probes/yy_load_buffer.cpp"
MANIFEST = ROOT / "vuc/config/yy_load_buffer_probe.json"

spec = importlib.util.spec_from_file_location("vuc_probe", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)

original = SOURCE.read_text()
start = original.index("void yyFlexLexer::yy_load_buffer_state()\n{")
prefix = original[:start]
assert original[start:].strip().endswith("}")

events = {
    "count": "int count = state->yy_n_chars;",
    "input": "std::istream *input = state->file;",
    "position": "char *position = state->yy_buf_pos;",
    "write_count": "yy_n_chars_ = count;",
    "write_input": "input_ = input;",
    "write_position": "yy_c_buf_p_ = position;",
    "write_text": "yytext_ptr_ = position;",
    "write_hold": "yy_hold_char_ = *position;",
}
writes = ("write_count", "write_input", "write_position", "write_text", "write_hold")
reads = ("count", "input", "position")
SCRATCH.mkdir(parents=True, exist_ok=True)
results = []
attempted = 0

for order in itertools.permutations(events):
    if tuple(step for step in order if step.startswith("write_")) != writes:
        continue
    if any(order.index(read) > order.index("write_" + write)
           for read, write in (("count", "count"), ("input", "input"),
                               ("position", "position"))):
        continue
    attempted += 1
    code = ("\n".join(("void yyFlexLexer::yy_load_buffer_state()", "{",
                        "    yy_buffer_state *state = current_;",
                        *("    " + events[step] for step in order), "}", "")))
    name = "-".join(str(order.index(read)) for read in reads)
    candidate = SCRATCH / ("load-" + name + ".cpp")
    obj = candidate.with_suffix(".o")
    candidate.write_text(prefix + code)
    invocation = subprocess.run([str(COMPILER), "-O2", "-frename-registers",
                                 "-c", str(candidate), "-o", str(obj)],
                                capture_output=True, text=True)
    if invocation.returncode:
        raise RuntimeError(invocation.stderr[-1500:])
    try:
        result = matcher.run(obj, MANIFEST)
    except ValueError:
        continue  # The candidate's body length differs from the original.
    results.append((result["exact_bytes"], result["length"], name,
                    result["exact"], str(candidate)))
    if result["exact"]:
        print("EXACT:", str(candidate), flush=True)

print("Compiled %d source-order variants; compared %d original-length candidates" %
      (attempted, len(results)))
for hit in sorted(results, reverse=True)[:15]:
    print("%d/%d %s %s" % hit[:4], hit[4])

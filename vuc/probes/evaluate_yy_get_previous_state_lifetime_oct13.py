#!/usr/bin/env python3
"""Recover yy_get_previous_state by testing the genuine Flex input-byte lifetime.

This 205-byte function shares signed scanner transition tables with
the independently exact yy_try_NUL_trans. An explicit local input byte
recovers the original load-before-NUL-initialization instruction order.
"""
from pathlib import Path
import importlib.util
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path("/tmp/vuc-binutils215/yy-previous-state-lifetime-oct13")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location(
    "matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)

source = (ROOT / "vuc/src/YyGetPreviousState.cpp").read_text()
assert "unsigned char yy_ch = *yy_cp;" in source
assert "if (yy_ch)\n            yy_c = yy_ec[yy_ch];" in source
alternatives = {
    "local-byte-exact": source,
    "repeat-input-read": source.replace(
        "        unsigned char yy_ch = *yy_cp;\n", "").replace(
        "if (yy_ch)\n            yy_c = yy_ec[yy_ch];",
        "if (*yy_cp)\n            yy_c = yy_ec[(unsigned char)*yy_cp];"),
    "unsigned-base": source.replace(
        "extern const short yy_base[];",
        "extern const unsigned short yy_base[];"),
}
config = ROOT / "vuc/config/yy_get_previous_state_oct13_probe.json"
flags = [
    ("jan25-default", "/tmp/vuc-binutils215/g++-jan25-as215",
     ["-O2", "-frename-registers"]),
    ("dec06-default", "/tmp/vuc-binutils215/g++-dec06-as215",
     ["-O2", "-frename-registers"]),
    ("jan25-j1", "/tmp/vuc-binutils215/g++-jan25-as215",
     ["-O2", "-frename-registers", "-falign-jumps=1"]),
    ("jan25-plain", "/tmp/vuc-binutils215/g++-jan25-as215",
     ["-O2"]),
]
for case, candidate in alternatives.items():
    path = OUT / (case + ".cpp")
    path.write_text(candidate)
    for label, compiler, options in flags:
        obj = OUT / (case + "-" + label + ".o")
        build = subprocess.run(
            [compiler, *options, "-c", str(path), "-o", str(obj)],
            capture_output=True, text=True)
        if build.returncode:
            print("COMPILE_FAILED", case, label, build.stderr[-125:], flush=True)
            continue
        try:
            result = matcher.run(obj, config)
            print("MATCH", case, label,
                  str(result["exact_bytes"]) + "/" + str(result["length"]),
                  "relocs=" + str(result["relocations"]),
                  "diff=" + str(result["different_offsets"][:12]), flush=True)
            if result["exact"]:
                print("BYTE_EXACT", case, label, flush=True)
        except ValueError as error:
            print("REJECT", case, label, str(error)[:110], flush=True)

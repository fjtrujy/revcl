#!/usr/bin/env python3
"""Verify recovered Flex NUL transition table signedness and ABI bytes.

Original VUC NUL transition accesses six genuine generated lexer tables.
An incorrect unsigned yy_base type gives 158/161, whereas the signed
short table gives 161/161 with original relocation targets.
"""
from pathlib import Path
import importlib.util
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = Path("/tmp/vuc-binutils215/yy-nul-signedness-oct13")
OUT.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location(
    "matcher", ROOT / "vuc/scripts/match_probe.py")
matcher = importlib.util.module_from_spec(spec)
spec.loader.exec_module(matcher)

base = (ROOT / "vuc/src/YyTryNulTrans.cpp").read_text()
config = ROOT / "vuc/config/yy_try_nul_trans_oct13_probe.json"
cases = {
    "signed-base-exact": base,
    "unsigned-base": base.replace(
        "extern const short yy_base[];", "extern const unsigned short yy_base[];"),
    "unsigned-def": base.replace(
        "extern const short yy_def[];", "extern const unsigned short yy_def[];"),
    "unsigned-check": base.replace(
        "extern const short yy_chk[];", "extern const unsigned short yy_chk[];"),
    "unsigned-next": base.replace(
        "extern const short yy_nxt[];", "extern const unsigned short yy_nxt[];"),
    "signed-accept": base.replace(
        "extern const unsigned short yy_accept[];", "extern const short yy_accept[];"),
    "unsigned-meta": base.replace(
        "extern const int yy_meta[];", "extern const unsigned int yy_meta[];"),
}
options = {
    "jan25-j1": ("/tmp/vuc-binutils215/g++-jan25-as215",
                 ["-O2", "-frename-registers", "-falign-jumps=1"]),
    "dec06-j1": ("/tmp/vuc-binutils215/g++-dec06-as215",
                 ["-O2", "-frename-registers", "-falign-jumps=1"]),
    "jan25-default": ("/tmp/vuc-binutils215/g++-jan25-as215",
                      ["-O2", "-frename-registers"]),
    "jan25-plain": ("/tmp/vuc-binutils215/g++-jan25-as215",
                    ["-O2", "-falign-jumps=1"]),
}
for variant, source in cases.items():
    path = OUT / (variant + ".cpp")
    path.write_text(source)
    for label, (compiler, flags) in options.items():
        obj = OUT / (variant + "-" + label + ".o")
        build = subprocess.run(
            [compiler, *flags, "-c", str(path), "-o", str(obj)],
            capture_output=True, text=True)
        if build.returncode:
            print("COMPILE_FAILED", variant, label, build.stderr[-160:], flush=True)
            continue
        try:
            result = matcher.run(obj, config)
            print("MATCH", variant, label,
                  str(result["exact_bytes"]) + "/" + str(result["length"]),
                  "relocs=" + str(result["relocations"]),
                  flush=True)
            if result["exact"]:
                print("BYTE_EXACT", variant, label, flush=True)
        except ValueError as error:
            print("REJECT", variant, label, str(error)[:125], flush=True)

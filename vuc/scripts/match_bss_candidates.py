#!/usr/bin/env python3
"""Strict source-declaration evidence for original VUC ELF32 .bss symbols.

This gate is deliberately NOT a byte scorer: SHT_NOBITS occupies zero bytes
in the executable file. It requires matching original ELF named symbol
identity, type, section and size plus matching GCC-generated source objects.
Final executable section alignment, BSS addresses and runtime state remain
unproven until a complete reproducible link exists.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
CONFIG = ROOT / "vuc/config/bss_candidates.json"
spec = importlib.util.spec_from_file_location(
    "vuc_match_data", ROOT / "vuc/scripts/match_data.py")
data = importlib.util.module_from_spec(spec)
spec.loader.exec_module(data)


def original_record(record, raw, target):
    if record.get("section") != ".bss":
        raise ValueError("only original .bss declarations are in scope")
    sym = data.original_symbol(raw, target, record["symbol"],
                               record["address"], record["size"], 1)
    if sym[3] != ".bss":
        raise ValueError("original named symbol is not in .bss")
    original_bss = next(s for s in target["sections"] if s["name"] == ".bss")
    if original_bss["type"] != 8:
        raise ValueError("original .bss is not SHT_NOBITS")
    if not original_bss["address"] <= sym[0] <= original_bss["address"] + original_bss["size"] - sym[1]:
        raise ValueError("original symbol lies outside .bss")
    if record["size"] < 1:
        raise ValueError("invalid source declaration size")
    return sym


def candidate_record(record, raw):
    sections, headers = data.match.elf_sections(raw)
    symbols = data.read_named_symbols(raw, sections, headers)
    found = [(name, sym) for name, sym in symbols
             if name == record["symbol"] and (sym[3] & 15) == 1
             and 0 < sym[5] < len(headers)]
    if len(found) != 1:
        raise ValueError("candidate must define exactly one STT_OBJECT: " + record["symbol"])
    _, entry = found[0]
    if entry[2] != record["size"]:
        raise ValueError("candidate symbol size differs from canonical ELF")
    section = headers[entry[5]]
    names = {index: name for name, (index, _) in sections.items()}
    name = names[entry[5]]
    if section[1] != 8 or not (section[2] & 0x3) == 0x3 or not (name == ".bss" or name.startswith(".bss.")):
        raise ValueError("candidate symbol is not in allocated writable SHT_NOBITS .bss")
    if entry[1] < 0 or entry[1] + entry[2] > section[5]:
        raise ValueError("candidate symbol outside SHT_NOBITS extent")
    return entry


def within_vuc_src(path):
    file = (ROOT / path).resolve()
    if not file.is_relative_to((ROOT / "vuc/src").resolve()) or not file.is_file():
        raise ValueError("candidate source must exist under vuc/src")
    return file


def run(compiler, out, records):
    if not records:
        raise ValueError("BSS manifest is empty")
    out = Path(out)
    out.mkdir(parents=True, exist_ok=True)
    raw, target = data.match.decomp.load()
    seen = set()
    compiled = {}
    result = []
    for record in records:
        key = (record["symbol"], int(record["address"], 0))
        if key in seen:
            raise ValueError("duplicate original BSS symbol/address identity")
        seen.add(key)
        original_record(record, raw, target)
        src = within_vuc_src(record["source"])
        flags = record["flags"]
        if not isinstance(flags, list) or not flags or not all(isinstance(f, str) and f.startswith("-") for f in flags):
            raise ValueError("missing explicit compiler flags")
        compile_key = (src, tuple(flags))
        if compile_key not in compiled:
            name = hashlib.sha256((str(src) + "\0" + "\0".join(flags)).encode()).hexdigest()[:12]
            obj = out / (src.stem + "-" + name + ".o")
            proc = subprocess.run([compiler, *flags, "-c", str(src), "-o", str(obj)],
                                  text=True, capture_output=True)
            if proc.returncode:
                raise RuntimeError("compiler failed for " + str(src) + ": " + proc.stderr[-1200:])
            compiled[compile_key] = obj
        candidate_record(record, compiled[compile_key].read_bytes())
        result.append({"symbol": record["symbol"], "original_address": record["address"],
                       "size": record["size"], "source": record["source"],
                       "declaration_match": True, "file_backed_bytes": 0})
    summary = {"scope": "named_original_NOBITS_BSS_source_layout_evidence_only",
               "status": "PASS", "declarations_checked": len(result),
               "declarations_matching": len(result),
               "verified_bss_allocation_bytes": sum(x["size"] for x in result),
               "matched_executable_file_bytes": 0,
               "linked_original_addresses_reproduced": False,
               "whole_binary_match": False, "results": result}
    (out / "bss_candidate_summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    return summary


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--compiler", required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--manifest", type=Path, default=CONFIG)
    a = p.parse_args(argv)
    records = json.loads(a.manifest.read_text())
    r = run(a.compiler, a.out, records)
    print("%d/%d matching original BSS symbol declarations; %d source allocation bytes"
          % (r["declarations_matching"], r["declarations_checked"],
             r["verified_bss_allocation_bytes"]))
    print("Linked whole-ELF byte match: NOT ESTABLISHED; BSS file-backed bytes: 0")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print("BSS-declaration gate failed: " + str(error), file=sys.stderr)
        sys.exit(1)

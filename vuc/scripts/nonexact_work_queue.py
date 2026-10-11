#!/usr/bin/env python3
"""Find bounded groups of original VUC functions not yet byte-exact.

All identities come from the authenticated original ZIP and the exact-only
manifest. This is a *source reconstruction work queue*, never a binary score:
unaligned text, stripped functions, padding, relocation/data bytes, and the
full linker image are deliberately not counted as matched.
"""
import argparse
import csv
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HERE = ROOT / "vuc"
CONFIG = HERE / "config"


def load_module(name):
    spec = importlib.util.spec_from_file_location("vuc_" + name, HERE / "scripts" / (name + ".py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def local_source_map(path):
    """Map *only* explicitly identified local ELF symbols to source files."""
    result = {}
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            for raw in row["local_text_addresses"].split(";"):
                if raw and raw != "-":
                    address = int(raw, 0)
                    if address in result:
                        raise ValueError("ambiguous original local TU ownership: 0x%08x" % address)
                    result[address] = row["file"]
    return result


def is_compiler_startup_name(symbol):
    """Name-based triage only; never a claim of original source ownership."""
    return (symbol.startswith("_GLOBAL__I_")
            or symbol.startswith("__tcf_")
            or symbol.startswith("__libc_csu_")
            or symbol in ("_start", "call_gmon_start", "frame_dummy",
                          "__do_global_dtors_aux", "__do_global_ctors_aux")
            or symbol == "_Z41__static_initialization_and_destruction_0ii")


def build_queue(rows, source_files, top=20):
    """Return original-address-bounded, never-overlapping nonexact runs."""
    ordered = sorted(rows, key=lambda record: record[0])
    if len({record[0] for record in ordered}) != len(ordered):
        raise ValueError("duplicate original text function addresses")
    groups = []
    current = []

    def finish():
        if not current:
            return
        funcs = [{"address": "0x%08x" % addr, "size": size,
                  "status": state, "symbol": symbol,
                  "source_file": source_files.get(addr)}
                 for addr, size, state, _, _, symbol in current]
        first, last = current[0], current[-1]
        groups.append({
            "start": "0x%08x" % first[0],
            "end_of_last_symbol": "0x%08x" % (last[0] + last[1]),
            "symbol_count": len(funcs),
            "symbol_bytes": sum(x["size"] for x in funcs),
            "functions": funcs
        })
        current.clear()

    for row in ordered:
        address, size, state, _, _, _ = row
        if state == "EXACT":
            finish()
        elif size > 0:
            if current:
                previous = current[-1]
                if address < previous[0] + previous[1]:
                    raise ValueError("overlapping original unresolved named function spans")
            current.append(row)
    finish()

    open_functions = [f for g in groups for f in g["functions"]]
    sizes = [f["size"] for f in open_functions]
    status = {
        "exact_named_function_records": sum(r[2] == "EXACT" for r in ordered),
        "experimental_named_function_records": sum(r[2] == "EXPERIMENTAL" for r in ordered),
        "unresolved_named_function_records": sum(r[2] == "UNRESOLVED" for r in ordered),
        "unresolved_positive_size_functions": len(open_functions),
        "nonexact_named_symbol_bytes": sum(sizes),
        "unresolved_runs": len(groups),
        "note": "NOT a binary score; original code/data/linker bytes not reconstructed from these counts."
    }
    short = sorted(open_functions, key=lambda f: (f["size"], int(f["address"], 0)))
    application = [f for f in short if not is_compiler_startup_name(f["symbol"])]
    big = sorted(groups, key=lambda g: (-g["symbol_bytes"], int(g["start"], 0)))
    return {"summary": status, "smallest_nonexact": short[:top],
            "smallest_nonexact_nonstartup": application[:top],
            "largest_nonexact_runs": big[:top]}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--top", type=int, default=12)
    p.add_argument("--json", type=Path, help="Explicit output file for a JSON report")
    args = p.parse_args()
    if args.top < 1:
        p.error("--top must be positive")

    decomp = load_module("decomp")
    coverage = load_module("coverage")
    _, info = decomp.load()
    exact = json.loads((CONFIG / "exact_candidates.json").read_text())
    selected = json.loads((CONFIG / "selected_candidates.json").read_text())
    rows = coverage.classify(info, exact, selected)
    result = build_queue(rows, local_source_map(CONFIG / "source_units.tsv"), args.top)
    if args.json:
        args.json.write_text(json.dumps(result, indent=2) + "\n")
    print("Exact: %(exact_named_function_records)d | experimental: "
          "%(experimental_named_function_records)d | unresolved: "
          "%(unresolved_named_function_records)d" % result["summary"])
    print("Nonexact, positive-size named symbols: %(unresolved_positive_size_functions)d "
          "across %(unresolved_runs)d address runs (NOT an ELF match score)" % result["summary"])
    print("\nShortest nonexact original named functions:")
    for f in result["smallest_nonexact"]:
        provenance = (" [%s]" % f["source_file"]) if f["source_file"] else ""
        print("%s %4d %-12s %s%s" % (f["address"], f["size"], f["status"], f["symbol"], provenance))
    print("\nShortest nonexact original functions other than named startup helpers:")
    for f in result["smallest_nonexact_nonstartup"]:
        provenance = (" [%s]" % f["source_file"]) if f["source_file"] else ""
        print("%s %4d %-12s %s%s" % (f["address"], f["size"], f["status"], f["symbol"], provenance))
    print("\nLargest nonexact runs (named function bytes only):")
    for g in result["largest_nonexact_runs"]:
        print("%s..%s %6d bytes across %d function symbols" %
              (g["start"], g["end_of_last_symbol"], g["symbol_bytes"], g["symbol_count"]))


if __name__ == "__main__":
    main()

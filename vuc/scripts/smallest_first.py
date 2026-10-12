#!/usr/bin/env python3
"""Prioritize VUC byte-exact reconstruction by ORIGINAL function size.

The queue is independently derived from the ZIP-backed ELF and exact manifest.
Local source ownership and disk probe counts are triage hints, NOT acceptance.
Zero-sized symbols have no independently verifiable function extent and are
listed separately. No byte-matching percentage is inferred from this queue.
"""
import argparse
from collections import Counter, defaultdict
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "vuc/scripts"
CONFIG = ROOT / "vuc/config"
TIERS = (32, 64, 128, 256, 512, 1024)


def module(name):
    spec = importlib.util.spec_from_file_location("vuc_smallest_" + name,
                                                 SCRIPTS / (name + ".py"))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def probe_index(paths):
    """Index hypotheses by original (address, symbol), never by name alone."""
    index = defaultdict(list)
    for path in paths:
        try:
            info = json.loads(path.read_text())
            key = (int(info["address"], 0), info["function"])
        except (OSError, ValueError, KeyError, TypeError):
            continue  # Invalid scratch input is not matching evidence.
        index[key].append(path.relative_to(ROOT).as_posix())
    return {key: sorted(value) for key, value in index.items()}


def classify_kind(symbol, accepted):
    if symbol.startswith("_GLOBAL__I_"):
        anchor = symbol[len("_GLOBAL__I_"):]
        return "startup:accepted-anchor" if anchor in accepted else "startup:unresolved-anchor"
    if symbol.startswith("__tcf_"):
        return "compiler:cleanup"
    if symbol.startswith("__libc_csu_"):
        return "runtime:startup"
    return "application"


def make_queue(rows, owners, probes, max_size=None):
    """Sort remaining sized original functions strictly by size then VA."""
    accepted = {row[5] for row in rows if row[2] == "EXACT"}
    pending = []
    unsized = []
    for address, size, status, _, _, symbol in rows:
        if status == "EXACT":
            continue
        if size == 0:
            unsized.append({"address": "0x%08x" % address, "symbol": symbol,
                            "reason": "original ELF symbol has zero st_size"})
            continue
        if max_size is not None and size > max_size:
            continue
        key = (address, symbol)
        hypothesis_paths = probes.get(key, [])
        pending.append({
            "address": "0x%08x" % address,
            "size": size,
            "status": status,
            "symbol": symbol,
            "kind": classify_kind(symbol, accepted),
            "original_local_source": owners.get(address),
            "probe_count": len(hypothesis_paths),
            "probes": hypothesis_paths,
            "tier": next((upper for upper in TIERS if size <= upper), None),
        })
    pending.sort(key=lambda row: (row["size"], int(row["address"], 16)))
    unsized.sort(key=lambda row: int(row["address"], 16))
    return {"pending": pending, "unsized": unsized,
            "tiers": dict(sorted(Counter(row["tier"] or "1025+" for row in pending).items(),
                                 key=lambda item: int(str(item[0]).rstrip("+")))),
            "note": "Priority/order only. NOT a byte-exact ELF score."}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--top", type=int, default=25)
    parser.add_argument("--max-size", type=int, help="optional strict size ceiling")
    parser.add_argument("--json", type=Path, help="write full deterministic queue")
    args = parser.parse_args()
    if args.top < 1 or (args.max_size is not None and args.max_size < 1):
        parser.error("--top and --max-size must be positive")

    _, info = module("decomp").load()
    exact = json.loads((CONFIG / "exact_candidates.json").read_text())
    selected = json.loads((CONFIG / "selected_candidates.json").read_text())
    rows = module("coverage").classify(info, exact, selected)
    owners = module("nonexact_work_queue").local_source_map(CONFIG / "source_units.tsv")
    probes = probe_index(CONFIG.glob("*probe.json"))
    report = make_queue(rows, owners, probes, args.max_size)
    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + "\n")

    print("Smallest-first: %d original sized, non-exact function symbols" %
          len(report["pending"]))
    print("Original zero-size symbols excluded from function matching: %d" %
          len(report["unsized"]))
    print("Size-tier counts:", report["tiers"])
    for row in report["pending"][:args.top]:
        print("%3d %s %-13s %-27s probes=%d %s%s" %
              (row["size"], row["address"], row["status"], row["kind"],
               row["probe_count"], row["symbol"],
               (" [" + row["original_local_source"] + "]")
               if row["original_local_source"] else ""))
    print(report["note"])


if __name__ == "__main__":
    main()

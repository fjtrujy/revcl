#!/usr/bin/env python3
"""Inventory every original VUC function as EXACT, EXPERIMENTAL or UNRESOLVED.

This is a symbol-level work queue, not a byte score or evidence of TU
ownership for global symbols. Only exact_candidates.json grants EXACT status.
"""
import argparse
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("vuc_decomp", ROOT / "vuc/scripts/decomp.py")
decomp = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decomp)
CONFIG = ROOT / "vuc/config"
OUTPUT = CONFIG / "function_status.tsv"


def classify(info, exact_paths, exploratory_paths):
    original = {(f["name"], f["address"], f["size"]): f for f in info["functions"]}
    accepted = {}
    exploratory = {}
    for paths, dest, status in (
        (exact_paths, accepted, "EXACT"),
        (exploratory_paths, exploratory, "EXPERIMENTAL"),
    ):
        for name in paths:
            manifest = ROOT / name
            config = json.loads(manifest.read_text())
            key = (config["function"], int(config["address"], 0), config["size"])
            if key not in original:
                raise ValueError("manifest does not identify an original function: " + name)
            if key in dest:
                raise ValueError("duplicate " + status + " manifest: " + name)
            if status == "EXACT" and not config["source"].startswith("vuc/src/"):
                raise ValueError("EXACT manifest must have accepted reconstruction source: " + name)
            if status == "EXPERIMENTAL" and key not in accepted and not config["source"].startswith("vuc/probes/"):
                raise ValueError("unaccepted experiment must live in vuc/probes/: " + name)
            dest[key] = (name, config["source"])
    result = []
    for f in info["functions"]:
        key = f["name"], f["address"], f["size"]
        if key in accepted:
            state, manifest, source = "EXACT", *accepted[key]
        elif key in exploratory:
            state, manifest, source = "EXPERIMENTAL", *exploratory[key]
        else:
            state, manifest, source = "UNRESOLVED", "-", "-"
        result.append((f["address"], f["size"], state, manifest, source, f["name"]))
    return result


def table(rows):
    lines = ["address\tsize\tstatus\tmanifest\tcandidate_source\tsymbol"]
    for address, size, state, manifest, source, symbol in rows:
        lines.append("0x%08x\t%d\t%s\t%s\t%s\t%s" %
                     (address, size, state, manifest, source, symbol))
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    _, info = decomp.load()
    exact = json.loads((CONFIG / "exact_candidates.json").read_text())
    exploratory = json.loads((CONFIG / "selected_candidates.json").read_text())
    result = classify(info, exact, exploratory)
    for state in ("EXACT", "EXPERIMENTAL", "UNRESOLVED"):
        print("%s: %d function symbols" % (state, sum(x[2] == state for x in result)))
    print("Whole-binary matching status: UNAVAILABLE")
    if args.write:
        OUTPUT.write_text(table(result))
        print("wrote " + str(OUTPUT.relative_to(ROOT)))
    else:
        print(table(result), end="")


if __name__ == "__main__":
    main()

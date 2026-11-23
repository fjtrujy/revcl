#!/usr/bin/env python3
"""Render all original VUC symbol-function byte scores from verified manifests.

Exact scores come from exact_candidates.json after historical candidate tests.
Partial scores come only from a passed historical partial-checkpoint report.
This is not a whole-linked-ELF byte-score or proof of exact final binary.
"""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
VUC = ROOT / "vuc"
CONFIG = VUC / "config"


def load_module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def demangle(names):
    if shutil.which("c++filt"):
        result = subprocess.run(["c++filt", "-n"], input="\n".join(names) + "\n",
                                capture_output=True, text=True, check=True)
        lines = result.stdout.splitlines()
        if len(lines) != len(names):
            raise ValueError("Demangler result count differs from symbols")
        return lines
    return names


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--partial-report", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    partial = json.loads(args.partial_report.read_text())
    if (partial["regressions"] or partial["whole_binary_score"] is not None or
            partial["exact_acceptance"] or
            partial["scope"] != "selected_nonexact_original_function_experiments"):
        raise ValueError("Not a valid passed partial checkpoint report")
    exact = json.loads((CONFIG / "exact_candidates.json").read_text())
    selected = json.loads((CONFIG / "selected_candidates.json").read_text())
    parts = {entry["manifest"]: entry for entry in partial["results"]}
    if (len(parts) != len(partial["results"]) or
            set(parts) != set(selected) - set(exact)):
        raise ValueError("Partial report does not match accepted/selected manifests")

    _, elf = load_module(VUC / "scripts/decomp.py", "vuc_d_report").load()
    rows = load_module(VUC / "scripts/coverage.py", "vuc_cov_report").classify(
        elf, exact, selected)
    names = demangle([row[5] for row in rows])
    exact_rows, partial_rows, unresolved_rows = [], [], []
    for row, name in zip(rows, names):
        addr, size, state, manifest, src, symbol = row
        display = chr(96) + name.replace(chr(96), "'") + chr(96)
        if state == "EXACT":
            if size <= 0:
                raise ValueError("Zero-sized symbol cannot be exact")
            exact_rows.append((addr, size, size, 0, display))
        elif state == "EXPERIMENTAL":
            entry = parts[manifest]
            if (entry["function"] != symbol or
                    int(entry["address"], 16) != addr or
                    entry["original_size"] != size):
                raise ValueError("Stale partial identity: " + manifest)
            score = entry.get("exact_bytes") if entry["status"] in (
                "PARTIAL", "EXACT_READY") else None
            partial_rows.append((addr, size, score,
                                 size - score if score is not None else None,
                                 display))
        else:
            unresolved_rows.append((addr, size, None, None, display))
    scored = [r for r in partial_rows if r[2] is not None]
    all_original_named_bytes = sum(r[1] for r in exact_rows + partial_rows + unresolved_rows)
    exact_original_named_bytes = sum(r[1] for r in exact_rows)
    if (len(exact_rows) != len(exact) or
            sum(r[2] for r in scored) != partial["verified_partial_bytes"] or
            sum(r[1] for r in scored) != partial["verified_original_function_bytes"]):
        raise ValueError("Manifest or partial score totals differ")

    lines = [
        "# VUC 2004.1 — original-function byte-match inventory",
        "",
        "Generated from the canonical VUC ELF symbol inventory, accepted exact",
        "function manifests, and a successful historical GCC 3.3.3",
        "relocation/literal-verified partial-checkpoint run.",
        "",
        "**Function scores do not measure the entire linked ELF.** No runnable",
        "or complete byte-exact VUC executable has been reconstructed yet.",
        "",
        "## Totals",
        "",
        f"- Named text symbols: **{len(rows)}**.",
        f"- **{len(exact_rows)}** exact functions, **{sum(r[1] for r in exact_rows):,}**"
        " verified exact instruction bytes.",
        f"- Exact functions occupy **{exact_original_named_bytes:,}/{all_original_named_bytes:,}** "
        f"original bytes across all named function extents (**{exact_original_named_bytes / all_original_named_bytes:.1%}**). "
        "This is a *strict accepted-function extent ratio*, NOT whole-ELF byte matching.",
        f"- **{len(partial_rows)}** selected experimental functions, "
        f"**{len(scored)}** eligible original-length partials matching "
        f"**{sum(r[2] for r in scored):,}/{sum(r[1] for r in scored):,}** bytes",
        "  within those partial functions only.",
        f"- **{len(unresolved_rows)}** unresolved symbols, including "
        f"**{sum(r[1] == 0 for r in unresolved_rows)}** zero-sized symbols.",
        "- Whole executable: **not yet buildable; no overall byte-match score**.",
        "",
        "## Experimental byte matches (sorted by missing instruction bytes)",
        "",
        "| Original address | Function | Original size | Verified bytes | Missing bytes |",
        "| --- | --- | ---: | ---: | ---: |",
    ]
    for addr, size, score, missing, name in sorted(
        partial_rows, key=lambda r: (r[3] is None,
                                    r[3] if r[3] is not None else 0, r[0])):
        lines.append(f"| {addr:#010x} | {name} | {size} | "
                     f"{str(score) + '/' + str(size) if score is not None else 'UNVERIFIED'}"
                     f" | {missing if missing is not None else '—'} |")
    lines += [
        "",
        "## Unresolved function symbols",
        "",
        "| Original address | Function | Original size | Verified bytes |",
        "| --- | --- | ---: | ---: |",
    ]
    for addr, size, _, _, name in sorted(unresolved_rows,
                                         key=lambda r: (r[1] == 0, r[1], r[0])):
        lines.append(f"| {addr:#010x} | {name} | {size} | — |")
    lines += [
        "",
        "## Accepted byte-exact function symbols",
        "",
        "| Original address | Function | Original size | Verified bytes |",
        "| --- | --- | ---: | ---: |",
    ]
    for addr, size, _, _, name in sorted(exact_rows):
        lines.append(f"| {addr:#010x} | {name} | {size} | {size}/{size} |")
    lines += [
        "",
        "This inventory is a dated verification snapshot. The historical source",
        "compiler and original vendor reference are obtained separately and",
        "are not committed. To regenerate: run the exact matcher, then the",
        "partial checkpoint matcher, then this report renderer.",
        "",
    ]
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(lines))
    print(f"{len(exact_rows)} exact, {len(partial_rows)} experimental, "
          f"{len(unresolved_rows)} unresolved; {sum(r[2] for r in scored)}/"
          f"{sum(r[1] for r in scored)} authenticated partial bytes")


if __name__ == "__main__":
    main()

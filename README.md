# REVCL

Modern reconstruction of Sony's historical PlayStation 2 VCL (Vector Unit
Command Line) optimizer/compiler tool.

The repository intentionally separates archived upstream artifacts from the
clean implementation:

- `archive/upstream/` preserves VCL, VUC, and VUEMU historical material.
- `docs/LEGACY_INVENTORY.md` records the initial technical analysis.
- `docs/REVCL_PLAN.md` is the phased reconstruction and validation plan.
- `tools/fetch_legacy_sources.py` reproducibly refreshes the archive.

Historical third-party files retain their original licenses and are not
relicensed by this repository's MIT license.

The first implementation milestone is underway: see
[`reverse/oracle/README.md`](reverse/oracle/README.md) for the historical VCL
test runner, Linux runtime setup, and initial behavioral fixtures.

## Matching decompilation of VCL 1.4 beta 7 (Linux x86)

A byte-for-byte matching C++ decompilation of
`archive/upstream/vcl/downloads/vcl.1.4.beta7.x86/vcl_14beta7_x86`
(GCC 3.2.2 Red Hat Linux 9, `-O2 -frename-registers`) lives in `src/`.

Progress (matched bytes / file size):

<!-- progress:start -->
**473,482 / 482,736 bytes (98.08%)** of `vcl_14beta7_x86` matched — code 265,299 / 268,717 (1184 / 1192 functions exact, 253,609 bytes), data 149,196 bytes, unwind tables 47,350 bytes, linker/runtime 11,637 bytes
<!-- progress:end -->

Setup and daily use:

```sh
docker build --platform linux/386 -t revcl-rh9 tools/rh9   # toolchain image
python3 -m venv .venv && .venv/bin/pip install -r scripts/requirements.txt
scripts/decomp status                  # totals + next targets
scripts/decomp diff 0x08058dd8         # incremental build + diff vs original
scripts/decomp lab 0x08080460          # local editable source + GCC ASM + original ASM workbench
scripts/decomp score --readme --html   # refresh this block and tmp/progress.html
.venv/bin/python scripts/iterate.py --rounds 5   # LLM agent loop
```

Open `tmp/progress.html` for the visual comparison of the original and the
rebuilt bytes and the progress history (`progress/history.csv`); its
`#run` tab ("Agent run") follows a running `iterate.py` live. Details: [`docs/decompilation.md`](docs/decompilation.md)
(approach, toolchain, scoring), [`docs/workflow.md`](docs/workflow.md) (agent
loop), [`docs/matching-notes.md`](docs/matching-notes.md) (compiler idioms),
[`AGENTS.md`](AGENTS.md) (rules for agents).

### Interactive source ↔ assembly experiments

The **[Assembly Workbench](docs/ASM_LAB.md)** is a permanent local browser
tool modeled on a three-pane decompilation editor: editable, syntax-highlighted
C/C++ source, relocation-resolved generated i386 assembly, and original
VCL assembly. Matching instructions are green; mismatches are red; missing
or extra instructions are amber. It offers a source-file explorer, synchronized
source/ASM scrolling, bidirectional source caret ↔ instruction highlighting
using code-verified historical GCC DWARF source lines, mismatch navigation,
quick original-GCC compilation,
compiler diagnostics, and a separate fixed-offset raw-byte score.

Run `scripts/decomp lab 0x08081610` to start experimenting with
`findInsertPositionMode0`, or pass any mapped original function address.
**Changes are saved only under ignored `tmp/asm_lab/`**, never automatically
into `src/`. The project comparison and full regression gates remain
authoritative for accepting a source change.

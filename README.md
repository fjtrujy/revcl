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
**215,762 / 482,736 bytes (44.70%)** of `vcl_14beta7_x86` matched — code 166,591 / 268,717 (837 / 1192 functions exact, 91,817 bytes), data 49,171 bytes
<!-- progress:end -->

Setup and daily use:

```sh
docker build --platform linux/386 -t revcl-rh9 tools/rh9   # toolchain image
python3 -m venv .venv && .venv/bin/pip install -r scripts/requirements.txt
scripts/decomp status                  # totals + next targets
scripts/decomp diff 0x08058dd8         # incremental build + diff vs original
scripts/decomp score --readme --html   # refresh this block and tmp/progress.html
.venv/bin/python scripts/iterate.py --rounds 5   # LLM agent loop
```

Open `tmp/progress.html` for the visual comparison of the original and the
rebuilt bytes and the progress history (`progress/history.csv`); its
`#run` tab ("Agent run") follows a running `iterate.py` live. Details: [`docs/decompilation.md`](docs/decompilation.md)
(approach, toolchain, scoring), [`docs/workflow.md`](docs/workflow.md) (agent
loop), [`docs/matching-notes.md`](docs/matching-notes.md) (compiler idioms),
[`AGENTS.md`](AGENTS.md) (rules for agents).

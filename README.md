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

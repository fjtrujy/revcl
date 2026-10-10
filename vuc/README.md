# VUC 2004.1 byte-matching reconstruction

This directory isolates the historical VUC reconstruction from the existing
VCL byte-matching project at the repository root.

## Target and baseline

The canonical input is the *tracked ZIP*, not the untracked extracted binary:

`archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip`
(member `X86linux_vuc2004_1/vuc`).

SHA-256: `14c77997057f279ab25a3591c6a379d6e4744f2b1551271d43602948c037d8cf`

- 314,338-byte ELF32 i386, dynamically linked and **not stripped**.
- 35 sections, including `.symtab`, `.strtab`, and limited DWARF.
- 716 static symbol entries (including data and non-function symbols).
- 298 named `.text` function symbols; these include weak aliases and runtime
  functions, and are **not** yet a deduplicated user-function inventory.
- 165,440 bytes in `.text` (including runtime startup/finalization code).
- Dependencies: `libstdc++.so.5`, `libgcc_s.so.1`, `libm.so.6`, `libc.so.6`.
- Two distinct GCC 3.3.3 Debian prerelease build dates occur in `.comment`:
  20031206 (4 tags) and 20040125 (24 tags). Whether these identify separately
  compiled VUC objects, bundled libraries, or startup code has **not** been established.
- All 298 named `.text` function symbols have distinct addresses; this is
  not yet a complete inventory of all functions in the executable.

Unlike the original VCL executable, VUC uses STL/iostream and retains symbols.
Do not assume the VCL GCC 3.2.2 flags, toolchain or matching behavior apply.

## First-stage commands

Requires Python 3.9+; `asm` additionally requires `objdump`.

```sh
python3 vuc/scripts/decomp.py inventory
python3 vuc/scripts/decomp.py inventory --write  # regenerate target.json/functions.tsv
python3 vuc/scripts/decomp.py status
python3 vuc/scripts/decomp.py info _Z23AsmOutput_WritePrologueRSt14basic_ofstreamIcSt11char_traitsIcEE
python3 vuc/scripts/decomp.py asm 0x08049b40
python3 -m unittest discover -s vuc/tests -v
```

The inventory reads and verifies the original bytes **inside the archive**.
`asm` uses a temporary extracted executable; it does not change the archive.
Regenerating the inventory must be deterministic.
The inventory also writes `vuc/config/sections.tsv`. See
`vuc/docs/toolchain-investigation.md` for the compiler evidence and open probes.

**No whole-binary matching percentage exists yet.** `status` intentionally reports
`UNAVAILABLE`, rather than misrepresenting a structural inventory as matched
bytes. An isolated relocation-aware **single-function** experiment is available
through `vuc/scripts/match_probe.py`; it is not the general `diff`/`score`
pipeline. A date-matched GCC 3.3.3 compiler-only toolchain now exists, but
the original Debian binaries and the complete scorer remain pending.

The existing x86_64 Lima VM can execute a separately extracted, SHA-pinned
GCC 3.3.5 compatibility toolchain (`vuc/scripts/bootstrap_compat_gcc.sh`).
This compiler is not an exact VUC toolchain. Its initial `-O2` hypothesis for
`AsmOutput_WritePrologue` matches 22/30 code bytes and 92/92 mapped literal
bytes; details, commands, and constraints are documented in
`vuc/docs/toolchain-investigation.md`.

A closer, date-matched **GCC 3.3.3 (20040125 prerelease)** compiler-only
toolchain is now reproducible from the original GCC branch revision via
`vuc/scripts/bootstrap_jan25_gcc.sh`. It successfully compiles six ELF32-i386
probes. The exact Debian `0pre3` binaries, downstream patches and historical
libstdc++ headers are not yet authenticated. See
`vuc/docs/toolchain-recovery-2026-10-10.md` for source hashes, VM commands,
build workarounds and compiler-flag experiments. Use
`vuc/scripts/compare_variants.py` for function-scoped optimization testing.

For multiple candidate functions, `vuc/scripts/audit.py --jobs /path/to/jobs.json`
accepts a JSON list of `{ "config": "/path/to/manifest.json",
"object": "/path/to/compiled.o" }` entries. Every candidate is validated
against the original function symbol, size, and address before comparison.
The audit reports checked bytes and exact candidate functions, **not** a
whole-program matching percentage. Multi-function object files are supported
as long as the requested function and its relocations can be resolved.

The **mandatory exact-only gate** is `vuc/config/exact_candidates.json`
(the default for `match_candidates.py`). It compiles the accepted source
fragments in `vuc/src/` and returns a nonzero exit status on **any** non-exact
function. Per-function compiler flags are recorded explicitly in the JSON
manifests; they are tested hypotheses, not authenticated original TU flags.
Run the strict gate using:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-gcc333-jan25/g++-jan25-candidate \
  --out /tmp/vuc-gcc333-jan25/candidate-suite
```

The strict gate now checks **11 independently verified exact functions**
covering 236/236 instruction bytes, including the original relocation for
`Instruction::ToString()`. The exploratory manifest
`vuc/config/selected_candidates.json` also includes the **unmatched** prologue;
it must fail when supplied with `--manifest`. Unmatched sources remain in
`vuc/probes/`, never in the accepted source tree. This is candidate-scoped
verification; final completion requires byte equality for the **entire VUC
binary**, not just its selected functions. See
`vuc/docs/exact-match-contract.md` for the acceptance contract.

The original ELF also supplies 22 `.cpp` translation-unit records and 79
locally scoped text-function records. Regenerate these with
`python3 vuc/scripts/source_units.py --write`. Global functions are not
assigned to source units merely from symbol-table ordering.

## Milestones

- **V0 — Inventory and isolation:** initial canonical manifest and entrypoint
  completed; still characterize all executable/data symbols and DWARF units.
- **V1 — Toolchain recovery:** reproduce compiler, assembler, linker, STL ABI,
  section layout and original flag set. Use probe sources for disambiguation.
  Never assume that the presence of a GCC version string proves the full flags.
- **V2 — Matching harness:** port the VCL relocation-aware linker/object
  comparison carefully, preserving VCL's existing working setup. Deduplicate
  aliases, distinguish runtime code, model translation units and initialize a
  component-wise score (code/data/unwind/linker/debug/symbols).
- **V3 — Reconstruction:** build plausible period C++ source in `vuc/src/`,
  map original translation units and class layouts using symbols and debug
  information, and match small functions first.
- **V4 — Iterative rounds:** use evidence-gated analyst/author/matcher/reviewer
  iterations, exact-function preservation, detailed attempts and commits.
- **V5 — Full fidelity:** complete source and binary-section matching; validate
  behavior with VUC fixtures, original VCL/MASP, and PCSX2 where relevant.

## Isolation and license

Do not modify the root VCL `src/`, `config/`, `scripts/`, or progress history
for VUC-only experiments. Do not stage the untracked extracted upstream trees
or unrelated local files. Never change historical archives.

VUC's historical user guide restricts redistribution of modified compiler
versions. Treat reconstructed source distribution as unresolved until reviewed;
the root repository's MIT license does not automatically settle those rights.

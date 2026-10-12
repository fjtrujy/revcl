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

The historical GNU **binutils 2.15** assembler is now reproducibly extracted
from a SHA-256-pinned Debian i386 package via
`vuc/scripts/bootstrap_binutils215.sh`. This resolves the original x86
alignment padding for `MLHashFun<char const*>::Hash()`. It is an
evidence-backed compatible assembler, **not** authenticated as the VUC
linker's original binutils release. See
`vuc/docs/historical-assembler-milestone.md` for the findings.

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
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-suite
```

The strict gate now checks **165 independently verified exact function-symbol records**
covering 11283/11283 instruction bytes, including the original relocation for
`Instruction::ToString()`. The exploratory manifest
`vuc/config/selected_candidates.json` also includes the **unmatched** prologue;
it must fail when supplied with `--manifest`. Unmatched sources remain in
`vuc/probes/`, never in the accepted source tree. This is candidate-scoped
verification; final completion requires byte equality for the **entire VUC
binary**, not just its selected functions. See
`vuc/docs/exact-match-contract.md` for the acceptance contract.

An independent **initialized-data acceptance gate** now checks
**9/9 original named `.data` objects, 68,364/68,364 bytes**,
including the 64 KiB `asmBuffer`, two 1 KiB text buffers, three
256-byte file/path buffers, and both original
`InstructionList` static counters (initialized to 1) and the
relocated `currFileName` pointer to an authenticated NUL literal.
It uses ordinary GCC 3.3 C++ global declarations and strict
ELF object/section-relocation validation. **One data relocation**
is resolved. These are **not function bytes**, and individual
object matches do **not** establish original section layout or
complete ELF identity. See
`vuc/docs/2026-10-12-exact-initialized-buffers.md` and run:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-nine-oct12
```
The original `StdAfx.cpp` startup wrapper is now byte-exact. The
matcher recognizes GCC 3.3's source-file-keyed random suffix without
relaxing the original symbol or relocation checks. See
`vuc/docs/2026-10-11-stdafx-source-unit.md`.
The two original `Value` bitwise operators now each reproduce their
complete 143-byte functions; see
`vuc/docs/2026-10-11-value-bitwise-batch.md`.
The reconstructed `Check(ASTFunCall*)` uses the historical implicit
`Type` copy constructor to reproduce all 118 instruction bytes.
See `vuc/docs/2026-10-11-ast-function-check-batch.md`.

The original ELF also supplies 22 `.cpp` translation-unit records and 79
locally scoped text-function records. Regenerate these with
`python3 vuc/scripts/source_units.py --write`. Global functions are not
assigned to source units merely from symbol-table ordering.
Regenerate the exhaustive **298-symbol** reconstruction queue using
`python3 vuc/scripts/coverage.py --write` (currently 165 EXACT,
9 EXPERIMENTAL and 124 UNRESOLVED). The complete original
`MLGameString` class member-function cluster is byte-exact (9/9 functions,
273/273 instruction bytes). The next string-data/hash-table milestone is
documented in `vuc/docs/string-data-hash-milestone.md`.
The latest batch adds exact `MLHashTable::FindCopy()`,
`MLTempStringData::Get()`, `SymbolTable::EndScope()`,
`yyFlexLexer::LexerOutput()` and `Value::Set(ASTExpr const*)`.
See `vuc/docs/2026-10-10-findcopy-tempdata-batch.md` for the C++
reconstruction and strict named-object relocation evidence.
The next stage also reconstructs `Semant_Check()` and three
`Check(ASTNodeList<T>&)` template specializations byte-exact.
See `vuc/docs/2026-10-10-semant-lists-batch.md`.
The following batch recovers virtual FlexLexer destructor entries,
`script_lex()`, `Translate(ASTNodeList<ASTVarDecl*>&)`, five
`Bind(ASTNodeList<T>&)` specializations, and one more hash-table cleanup.
See `vuc/docs/2026-10-10-lexer-bind-batch.md`.
The independent `IsFoldable(IR::Operator_Data*)` branch-chain
reconstruction also matches all 41 original instruction bytes.
The original `IsFoldable` startup translation unit is now reconstructed
coherently with both 513-bucket hash environments: its 193-byte static
initializer and 31-byte global initialization wrapper are fully exact.
See `vuc/docs/2026-10-10-predicate-environment-unit.md`.
The separate original `currFileName` translation unit's 88-byte
static initializer is also exact, while its 31-byte global wrapper
remains explicitly experimental at 29/31. See
`vuc/docs/2026-10-10-curr-file-and-whole-elf-gate.md`.
The original `InstructionList::GenTmp()` and both historical
`AsmVariable` STL uninitialized-copy helpers now match exactly using
ordinary C++ source; see `vuc/docs/2026-10-10-asmvariable-gentmp-batch.md`.
The matching suite also includes byte-exact `GenLabel()` and
`yyFlexLexer::yy_delete_buffer()`; their original relocations and
named data objects are checked in the same strict manifest.
The independent final executable gate is
`python3 vuc/scripts/whole_elf_gate.py --candidate /path/to/rebuilt-vuc`.
It accepts only complete byte equality with the authenticated ZIP
member (including code, data, ELF metadata and padding). No reconstructed
linked ELF has passed that gate yet; acceptance of individual
function records does not imply whole-executable equality.
Twenty per-translation-unit iostream destructor callbacks and
`_GLOBAL__I_constantsTabStop` are also individually byte-exact.
All 14 matching 66-byte iostream initializer helpers have now been
reconstructed, including the `constantsTabStop` helper. These
35 accepted startup/teardown function records are not 35 separate
reconstructed application algorithms. See
`vuc/docs/2026-10-10-iostream-startup-batch.md`.
The latest application-code batch adds 13 exact functions in the
hash-table, IR predicate, instruction-list, AST printer, numeric
conversion and FlexLexer subsystems. See
`vuc/docs/2026-10-10-application-code-batch.md`.
The additional `AlignConstant()` and
`yyFlexLexer::yy_switch_to_buffer()` functions are also exact.
`MLGameStringTable::Get()` now verifies named original static BSS
relocations, but remains non-exact at 94/106 bytes.
The current checkpoint adds the exact `yyFlexLexer::yyrestart()`
and two GCC 3.3 `std::__uninitialized_copy_aux` instantiations
for `Type`. See `vuc/docs/2026-10-10-restart-stl-copy-batch.md`.
The next batch exactly reconstructs scoped symbol lookup, both
`ListFile` printing methods, both global hash environment teardown
callbacks, and `AssembleModule()`. See
`vuc/docs/2026-10-10-symbol-list-environment-batch.md`.
The byte-exact 117-byte `GetOpType(IR::Expr_Data*)` function and a
separate recovered December 2003 GCC compiler are documented in
`vuc/docs/2026-10-10-get-op-type-and-dec06-toolchain.md`.
The following exact batch adds `InstructionList::Print()`,
`InstructionList::AddL()`, `Check<ASTExpr*>()`, `main()`,
the AST arena cleanup callback and `TranslateFrag()`. See
`vuc/docs/2026-10-10-entry-ir-constructors-batch.md`.
The lexer initialization/flush methods are now also exact, and the
relocation matcher correctly distinguishes defined STT_FUNC bases
from section-relative offsets. See
`vuc/docs/2026-10-10-lexer-buffer-batch.md`.
Both historical `yyFlexLexer` constructor ABI variants (C1/C2) are
now exactly reconstructed, **138/138 bytes each**, from the original
field-initialization sequence and named vtable relocation. See
`vuc/docs/2026-10-12-flex-constructors.md`.

**DWARF limitation:** the canonical executable contains four compilation
units, all belonging to glibc startup (`start.S`, `init.c`, `crti.S`,
`crtn.S`); there are **no retained VUC application DWARF C++ units**.
The ELF symbol table is a separate and more useful source of VUC function
names. Run `python3 vuc/scripts/dwarf_inventory.py --write` to regenerate
the original ZIP-backed DWARF provenance, and see
`vuc/docs/dwarf-evidence-and-get.md`. `MLGameStringTable::Get()` is
under experimental source reconstruction, not accepted as byte-exact.

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

For the next exact source-reconstruction batch, use the read-only
original-address work queue:

```sh
python3 vuc/scripts/nonexact_work_queue.py --top 15
```

It lists small non-exact application functions and larger symbol-order
unaccepted ranges, reporting gaps separately without claiming a binary score
or guessing global source-unit ownership. See
`vuc/docs/2026-10-11-nonexact-code-queue.md`.

The FlexLexer buffer-state reload remains non-exact after testing 15
ordinary C++ load/store orders with the original ELF comparator. See
`vuc/docs/2026-10-11-lexer-load-order-investigation.md` for reproducible
register-lifetime evidence; none of those candidates is accepted.
The separate original `AsmOutput_TabConstants()` constant-width scan
has a source/relocation/literal-complete 118/127-byte hypothesis,
but is also not accepted; see
`vuc/docs/2026-10-11-asm-output-tab-constants-investigation.md`.

Do not modify the root VCL `src/`, `config/`, `scripts/`, or progress history
for VUC-only experiments. Do not stage the untracked extracted upstream trees
or unrelated local files. Never change historical archives.

VUC's historical user guide restricts redistribution of modified compiler
versions. Treat reconstructed source distribution as unresolved until reviewed;
the root repository's MIT license does not automatically settle those rights.

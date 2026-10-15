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

The strict gate now checks **195 independently verified exact function-symbol records**
covering 13992/13992 instruction bytes, including the original relocation for
`Instruction::ToString()`. The exploratory manifest
`vuc/config/selected_candidates.json` also includes the **unmatched** prologue;
it must fail when supplied with `--manifest`. Unmatched sources remain in
`vuc/probes/`, never in the accepted source tree. This is candidate-scoped
verification; final completion requires byte equality for the **entire VUC
binary**, not just its selected functions. See
`vuc/docs/exact-match-contract.md` for the acceptance contract.

The most recent exact milestone reconstructs the genuine Printer.cpp
source-unit context, producing both `Printer_UglyPrint()` at **40/40**
and its generated iostream initialization wrapper at **31/31**.
See `vuc/docs/2026-10-13-printer-source-unit-exact-batch.md` for
the historical compiler and original ELF relocation/literal evidence.
The next AST.cpp milestone reproduces its **145/145** static
initialization helper and **31/31** `matchStack` startup wrapper in
one original-shaped C++ unit, while retaining the already exact 16-byte
iostream cleanup callback. See
`vuc/docs/2026-10-13-ast-matchstack-static-init.md`.
The following application-code milestone also resolves
`Canonize2_Reduce()` **85/85** by compiling the three genuine, already
byte-exact `ConvVal` overloads before it. See
`vuc/docs/2026-10-13-canonize2-conversion-context.md`.
The `Value.cpp` source-unit reconstruction similarly recovers the
31/31-byte `Value` global initialization wrapper alongside genuine
arithmetic and comparison members; see
`vuc/docs/2026-10-13-value-startup-context-batch.md`.
The `vuc_compiler.cpp` context also recovers the 31/31-byte
`currFileName` global startup wrapper with genuine scanner,
diagnostic and `main` methods; see
`vuc/docs/2026-10-13-currfile-diagnostic-unit-batch.md`.
The reconstructed SymbolTable source unit now also reproduces the
31/31-byte original `SymbolTable::Enter` global startup wrapper while
preserving exact `Find` and `BeginScope` functions. The `Enter` body
itself remains partial; see
`vuc/docs/2026-10-13-symboltable-enter-source-unit.md`.
The reconstructed ListFile source-unit context likewise yields the
original `ListFile::Generate` global startup wrapper at 31/31 bytes,
with independently verified neighboring ListFile methods. The long
`Generate` method itself remains partial; see
`vuc/docs/2026-10-13-listfile-generate-source-unit.md`.
The original semantic-analysis startup wrapper now matches 31/31 bytes
in a real `CheckActualParamList`, `Check(ASTFunCall*)`, and
`Semant_Check` source-unit context. The full argument validator remains
provisional; see `vuc/docs/2026-10-13-semant-startup-source-unit.md`.
The original `Translate(Type const&, unsigned int, int)`-keyed startup
wrapper is also 31/31 from an IR type-dispatch source-level hypothesis.
See `vuc/docs/2026-10-13-semant-translate-startup-batch.md` for both
newly accepted startup functions and their incomplete application bodies.
Near-exact `Printer` function-call printing, `Instruction` arena cleanup,
lexer buffer loading, and hash-node insertion now have bounded,
source-level negative controls; see
`vuc/docs/2026-10-13-printer-instruction-lexer-controls.md`.
The failed original-era AST `matchStack` container alternatives and the
unfixed `currFileName` wrapper after a real entry-point neighbor are
retained as reproducible controls in
`vuc/docs/2026-10-13-startup-source-context-controls.md`.
The October 13 rebase on VCL `main` at `1825ea7` incorporated the new
`link_oParts` GCC register-lifetime and native linker-layout evidence.
Five more original VUC startup records are now independently 31/31:
InlineAsm, IR2Asm BroadcastMove, IR2AsmBinExpr, script parser debugging,
and the Flex scanner. Their large application bodies remain provisional.
See `vuc/docs/2026-10-13-main-rebase-five-startup-functions.md` for
historical compiler controls, relocations, and linked-ELF limitations.
The next milestone reconstructs a full **151/151-byte**
`Type::IsAssignable(Type const&, bool) const` application method using
normal nested C++ compatibility checks and GCC 3.3.3 four-byte jump
alignment. See `vuc/docs/2026-10-13-type-isassignable-exact-batch.md`.
The same application-code batch reconstructs lexer buffer loading
**39/39** by recovering the actual five-field assignment order, and
`MLGameStringTable::Get()` **106/106** by retaining real source-unit
predecessors and resolving its destructor callback through a genuine
named `.text.__tcf_0` relocation. See
`vuc/docs/2026-10-13-lexer-load-and-string-table-exact.md`.
Two genuine `InstructionList` variadic application methods also match:
`Add(char const*, ...)` **146/146** and `AddB(char const*, ...)`
**146/146**, each with seven authenticated ELF32 relocations. Their
adjacent `AddLineComment(int)` is preserved as a **147/162** partial
with all relocations and literal bytes verified. See
`vuc/docs/2026-10-13-instruction-variadic-exact-batch.md`.
The authentic AsmOutput compilation-unit predecessor order now makes
`AsmOutput_TabConstants(InstructionList&)` **127/127**, with seven
original relocations and **127 verified source-unit string-literal
bytes** including the genuine `AlignConstant` method. `AlignConstant`
also remains **69/69** in the same source object, and `AlignString` is
tracked as **143/145** with eight relocations, rather than being promoted.
Standalone TabConstants code was 118/127. These remain per-function claims,
not a complete exact AsmOutput.cpp or linked ELF. The real-context
controls also record a 131/139 Flex buffer allocator, a 155/161 parser
diagnostic, and a 61/77 accumulator check as experimental functions.
See `vuc/docs/2026-10-13-asmoutput-tabconstants-exact-and-bounded-controls.md`.
The latest rebase incorporates `main` at `cd35047` (VCL `Operand::Emit`
reconstruction), including its GCC 3.2.2 lexical switch-case order and
integer-predicate evidence. Bounded VUC GCC 3.3.3 controls did not
reproduce those techniques unchanged. See
`vuc/docs/2026-10-13-operand-main-rebase-and-asm-source-unit.md`.

An independent **named-data acceptance gate** now checks
**244/244 original named initialized-data, RTTI, vtable and ABI objects,
71,536/71,536 bytes**,
including the 64 KiB `asmBuffer`, two 1 KiB text buffers, three
256-byte file/path buffers, and both original
`InstructionList` static counters (initialized to 1) and the
relocated `currFileName` pointer to an authenticated NUL literal.
The additional 81 original `.rodata` RTTI class-name objects and
**81 original ABI type-information objects** are generated by
historical GCC 3.3 from ordinary C++ `typeid` declarations and
**72 authenticated single-inheritance relationships**.
Nine typeinfo objects are root classes.
It uses ordinary GCC 3.3 C++ global declarations and strict
ELF object/section-relocation validation. **390 data relocations**
are resolved. These are **not function bytes**, and individual
object matches do **not** establish original section layout or
complete ELF identity. The class inheritance links are supported by
the original typeinfo objects; virtual methods, vtable layouts and
instance fields remain unverified.
See `vuc/docs/2026-10-12-exact-initialized-buffers.md`,
`vuc/docs/2026-10-12-rtti-inheritance-objects.md`, and
`vuc/docs/2026-10-12-flexlexer-vtable.md`, and
`vuc/docs/2026-10-12-inherited-vtables.md`, and
`vuc/docs/2026-10-12-runtime-and-streambuf-abi.md`, then run:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-typeinfo-171-oct12
```
The additional **uninitialized source-layout gate** verifies 12/12
canonical ELF `.bss` symbol declarations (18,437 bytes of allocated
storage) against historically compiled C++ source. Unlike initialized
data, `.bss` is `SHT_NOBITS` and has **zero file-backed bytes**:
these declarations must never inflate byte-exact data or function
coverage. Original linked allocation addresses and whole-ELF
identity remain unverified. See
`vuc/docs/2026-10-12-bss-source-declarations.md` and run:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_bss_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/bss-oct12-confirm
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
`python3 vuc/scripts/coverage.py --write` (currently 195 EXACT,
15 EXPERIMENTAL and 88 UNRESOLVED). The complete original
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
The recovered `inputFileName` translation-unit context makes its
31-byte initializer wrapper byte-exact **together with** the already
accepted 133-byte `ListFile::PrintLine()` and 256-byte global.
See `vuc/docs/2026-10-12-inputfilename-original-unit.md`.

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
The next source-level probes for `InstructionList::AddLineComment()` and
the 513-bucket hash-scope STL copy helper are also non-exact. Their
original-size comparisons, relocation evidence, and bounded compiler
experiments are preserved in
`vuc/docs/2026-10-12-line-comment-and-scope-copy-batch.md`.
The additional 30-byte prologue stream insertion, `GetNextWord`,
formatted-instruction insertion, default string-constructor source unit,
and `Value` addition probes also remain non-exact. See
`vuc/docs/2026-10-12-stream-and-constructor-probes.md` for bounded
compiler comparisons and original-symbol evidence.
An additional genuine `Instruction`-method/lifecycle combined-unit
control preserves four exact neighbors but leaves `__tcf_1` at 45/46;
see `vuc/docs/2026-10-13-instruction-unit-follow-up.md`.
After rebasing onto the latest VCL `main`, the transferable GCC source-
reconstruction techniques were reviewed with explicit GCC 3.3 caveats.
The VUC compiler's `-da` RTL dumps and a verified register-renaming
diagnostic are documented in
`vuc/docs/2026-10-13-vcl-main-technique-review.md`; none of the VCL-
specific compiler or source-unit exceptions is assumed to apply to VUC.
Further bounded historical-source controls for the real AsmOutput unit,
AST function-call printer, RTTI-backed sequence translation, and glibc
startup are recorded in
`vuc/docs/2026-10-13-source-provenance-bounded-batch.md`.
These are reproducible negative experiments and source-provenance findings,
not newly accepted byte-exact functions.
After the VCL `main` rebase at `bbe5056`, the restored real `Value::operator-`
source-unit predecessor makes `Value::operator*` **222/222 byte-exact**
under historical GCC 3.3.3, while preserving the **221/221** subtraction.
See `vuc/docs/2026-10-13-operand-main-rebase-and-value-arithmetic-exact.md`
for the genuine C++ source, relocation verification and negative controls.
The next milestone reconstructs the original 220-byte
`Value::TypeConvert()` **220/220 exact**, using genuine 32-byte C++
Value assignments and a shared failure return after nested switches.
Its isolated historical source and bounded negative source-unit controls
are documented in `vuc/docs/2026-10-13-typeconvert-exact-and-source-context.md`.
The following partial source-context batch reconstructs
`Value::ToString()` at **159/221** bytes with all eight relocations
and 82 initialized literal bytes authenticated, and improves
`Value::operator/()` to **178/235** bytes with its real TypeConvert
relocation. Both remain experimental, not accepted as exact; see
`vuc/docs/2026-10-13-value-tostring-and-division-partials.md`.
The next source-level control batch reconstructs the 257-byte original
`Value::TypeConvertForce()` as a **193/257** experimental partial with
its genuine preceding TypeConvert definition. Two separate GCC 3.3.3
build-date controls and typed arena records still leave `AlignString`
at **143/145** and the `Instruction` destructor at **45/46**.
See `vuc/docs/2026-10-13-forced-conversion-and-near-exact-controls.md`.
The recovered Flex NUL transition `yyFlexLexer::yy_try_NUL_trans(int)`
is now **161/161 byte-exact** with all eight original table relocations.
The key evidence is the original signed `short yy_base[]` table, which
restores the three missing sign-extension bytes without code forcing.
See `vuc/docs/2026-10-13-flex-nul-transition-byte-exact.md`.
The neighboring `yyFlexLexer::yy_get_previous_state()` is also
**205/205 byte-exact** with all nine original table relocations.
Caching the input character in a genuine local variable before the
NUL initialization recovers seven GCC-scheduled bytes; the same signed
scanner tables independently explain both functions. See
`vuc/docs/2026-10-13-flex-previous-state-byte-exact.md`.
Two additional genuine Flex buffer methods are now exact:
`yyFlexLexer::yyinput()` **170/170** through the original
`yyrestart`/EOF switch fallthrough, and `yyFlexLexer::yyunput()`
**167/167** with its authenticated original 32-byte fatal-error
literal. See `vuc/docs/2026-10-13-flex-input-unput-byte-exact.md`.
After rebasing all 101 VUC commits onto VCL `main` at `a0b6acf`,
the previously unresolved Flex `yy_get_next_buffer()` now has a
genuine **430/550** original-length experimental C++ reconstruction,
with `realloc` relocation and three authenticated original error
strings. It is not yet exact; original source-unit padding/layout
provenance remains unresolved. See
`vuc/docs/2026-10-13-rebase-main-flex-next-buffer-partial.md`.
Additional genuine compiler/source controls for this 430/550 partial,
plus two unresolved SymbolScope uninitialized-copy templates, found no
new exact bytes. Negative evidence and reproduction commands are retained
in `vuc/docs/2026-10-13-flex-followup-compiler-negative-controls.md`.
The next rebase onto VCL `main` at `ea1221c` provides genuine deferred
implementation-owner evidence. Applying the source-context lesson to
VUC recovers `AlignString()` **145/145 byte-exact**, including all eight
original PLT/BSS relocations, from a minimal ordinary C++03 file;
the earlier AsmOutput source-unit probe had only 143/145. See
`vuc/docs/2026-10-13-visitcache-main-rebase-alignstring-exact.md`.
The subsequent rebase onto VCL `main` at `8d609de` preserves all 195
accepted functions. The new deferred `dumpPathKey` compiler-state
evidence motivated bounded genuine Instruction arena method-order,
Flex buffer alignment and AST statement-list optimizer controls; none
improved their existing byte scores. See
`vuc/docs/2026-10-13-dumppathkey-rebase-negative-controls.md`.
The follow-up RTL pass study reproduces the one-byte Instruction
arena destructor mismatch (45/46) in both original GCC 3.3.3 snapshots:
the generated dead stack-discard target is EBX rather than original
ECX. Real printer source-order and deferred-ownership controls retain
96/98, while 180 Seq_Data constructor/loop variants cannot reproduce
the original 119-byte translator. See
`vuc/docs/2026-10-13-rtl-printer-ir-negative-batch.md`.
The next Value addition alignment investigation distinguishes the original
unpadded vector case from the scalar case aligned at +0xd0; 80 genuine
GCC alignment combinations retain the best 172/219 original-length match.
The C++ word-scanner experiments reproduce byte-sized delimiter comparisons
but still lack the original pointer/position register allocation. See
`vuc/docs/2026-10-14-value-plus-word-scanner-controls.md`.
After rebasing on VCL `main` at `1aadee5`, the reusable compiler,
source-layout, historical-provenance, and exact-match lessons from all
19 VCL matching-evidence reports and the recent 13 commits are indexed
in `vuc/docs/2026-10-14-vcl-main-byte-matching-playbook.md`. VCL
GCC 3.2.2 compiler quirks and authorized speculative source-unit
partitions remain hypotheses, not assumptions about VUC GCC 3.3.3.
The following structural-control batch tested genuine Value return/case
topologies, division x87 lifetimes, Flex buffer ownership, printer inline
getters, and MLTempString real predecessor methods with the historical VUC
compiler. None improved the existing partial scores; the reproducible
negative controls and exact byte offsets are recorded in
`vuc/docs/2026-10-14-structural-control-and-real-prefix-negative-batch.md`.
The separate `vuc-unresolved-oct14` branch explores the previously
unresolved 240-byte `TranslateFragList` specializations and 513-bucket
MLHashTable/STL copying. See
`vuc/docs/2026-10-14-unresolved-fragment-scope-batch.md` for authentic
template-source hypotheses, historical compiler measurements and remaining
source-unit/register-allocation mismatches; none is accepted as exact.
The next unresolved-only batch recovered the original 193-byte
`AssembleBinOp` dispatcher ABI, four specialist/default branches and all
four call relocations. A GCC 3.3.3-only compiler-frequency hypothesis
matches **162/193** bytes; it is deliberately kept in `vuc/probes/` rather
than accepted source because original authoring is not authenticated.
See `vuc/docs/2026-10-14-assemble-binop-unresolved-partial.md` for the
complete compiler matrix and an independent statement-list negative sweep.
The subsequent unresolved-only batch identified the original AST expression
list's 0x3c00 translation argument and fragment canonicalization's 513-bucket
hash-node constructor/exception lifetime. Two original-length diagnostic
matches, **90/256** and **78/224**, remain in `vuc/probes/`, not the exact
candidate set. See `vuc/docs/2026-10-14-exprlist-canonize-fragment-partials.md`.
The renamed `vuc` branch then recovered a new **263/263-byte exact**
`MLGameStringTable::Add(const char*, bool)` at `0x08071880`, including
all ten original relocation sites. The decisive genuine C++03 source
is a named `const char* const&` alias of the string-data pointer before
hash-node allocation; see
`vuc/docs/2026-10-14-mlgamestringtable-add-byte-exact.md`.
The subsequent small-function control batch tests the 30-byte output
prologue, MLTempString default C2, operator accumulator classification,
and 513-bucket copy construction with historical GCC 3.3.3. No new exact
function was recovered; reproducible source/branch/stack-lifetime
negative evidence is preserved in
`vuc/docs/2026-10-14-small-function-lifetime-negative-batch.md`.
After rebasing `vuc` onto VCL `origin/main` at `60d0b9a`, the newest
VCL guard-arm and branch-likelihood evidence unlocked the previously
147/162 partial `InstructionList::AddLineComment(int)` at **162/162**
with historical GCC 3.3.3 (`-falign-jumps=8`). All 7 relocation sites
and 22 original literal bytes match. The separately compiled accepted
source unit preserves existing InstructionList literal-section matches.
See `vuc/docs/2026-10-14-line-comment-exact-after-vcl-rebase.md`.
After rebasing once more onto VCL `origin/main` at `1693559`, joint
historical-GCC branch-likelihood controls improve the experimental
`IsAccumulatorable(IR::Operator_Data*)` from **61/77** to **74/77**
original bytes. The three residual bytes are the final conditional
branch orientation and the following unconditional displacement;
this is **not byte-exact** and is excluded from the exact manifest.
All 197 already exact VUC functions remain accepted. See
`vuc/docs/2026-10-14-vcl-setlocks-accumulator-74-byte-partial.md`.
The next historical-compiler batch recovered the independent
GCC 3.3.3 2003-12-06 prerelease again and rechecked the five most
promising nonexact functions under genuine inline-predicate,
constructor-prefix and weak-binding controls. **No new exact match**:
the 74/77 accumulator prefix, 45/46 arena cleanup, 96/98 AST printer,
95/101 temporary-string C2 and 111/117 temporary-string Set remain
unaltered. The reproducible negative evidence is retained in
`vuc/docs/2026-10-14-historical-snapshots-accumulator-compiler-controls.md`.
The next unresolved-function batch retained an original-length **60/119**
partial for `Translate(ASTNodeList<ASTStmt*>&)` with all three real
relocations checked. It is tracked as EXPERIMENTAL, not byte-exact;
the ledger becomes **197 EXACT / 15 EXPERIMENTAL / 86 UNRESOLVED**.
The same batch corrected `Canonize_Reduce(IR::Fragment_Data*)`'s
provisional statement-field offset from +0x14 to the original +0x18;
its independently verified partial remains 78/224. See
`vuc/docs/2026-10-14-stmtlist-partial-and-fragment-abi.md`.
The subsequent partial-first batch authenticated four already studied,
original-sized sources against the original ELF and formally tracks their
nonexact results: `Value::operator+()` **172/219**, `AssembleBinOp`
**162/193**, AST expression-list `Translate` **90/256**, and fragment
`Canonize_Reduce` **78/224**; all 18 associated relocation sites are
authenticated. The ledger is **197 EXACT / 19 EXPERIMENTAL /
82 UNRESOLVED**. No new exact function was accepted. A separate
`VFormatString` literal audit verified five original source strings
and rejected candidates with unmapped literal alignment gaps. See
`vuc/docs/2026-10-14-four-tracked-partials-and-vformat-literal-gate.md`.
The following AST expression-list source/ABI batch improved the selected
experimental `Translate(ASTNodeList<ASTExpr*>&)` candidate from
**90/256** to **112/256**, with both genuine ELF relocations checked.
This is a 22-byte improvement, **not byte-exact**, and does not change
the accepted manifest or the **197 EXACT / 19 EXPERIMENTAL /
82 UNRESOLVED** ledger. See
`vuc/docs/2026-10-14-expr-list-112-byte-partial.md`.
The next batch recovered **byte-exact `__libc_csu_init` (84/84)** from
ordinary C glibc startup source, compiled **together with already exact
`__libc_csu_fini` (80/80)** in the original unit. Both historical GCC
3.3.3 snapshots match both functions under `-xc -O2 -fPIC` with all
ten original PIC relocation sites authenticated. The ledger is now
**198 EXACT / 19 EXPERIMENTAL / 81 UNRESOLVED**, with **14,501**
accepted exact instruction bytes. An independent `__builtin_memcpy`
probe also recovered GCC's `cld; rep movsl` copy idiom in ASTExpr
translation but no original-length improvement over 112/256. See
`vuc/docs/2026-10-14-libc-csu-init-byte-exact.md`.
The next partial-focused batch independently rechecked and registered
two original-sized `TranslateFragList` template instantiations,
`ASTVarDecl*` and `ASTDecl*`, **41/240** bytes and **2/2** authenticated
relocations each. They are EXPERIMENTAL, not newly exact. Ledger:
**198 EXACT / 21 EXPERIMENTAL / 79 UNRESOLVED**. See
`vuc/docs/2026-10-14-fragment-list-two-partials.md`.
The next AST list-source transfer batch improved both selected fragment-list
specializations from **41/240** to **110/240** exact original-offset
instruction bytes each, with **2/2** authenticated call relocations
per function. The 138-byte total improvement is experimental,
**not** two newly byte-exact functions; the source still uses a
scalar copy rather than the original `rep movsl`. The ledger remains
**198 EXACT / 21 EXPERIMENTAL / 79 UNRESOLVED**. See
`vuc/docs/2026-10-14-fragment-list-110-byte-partials.md`.
The subsequent historical C++ STL-copy batch authenticated a **75-byte
consecutive exact prefix** (and 84 matching original-offset bytes) of
the unresolved 120-byte `__uninitialized_copy_aux<Scope*, Scope*>`
at `0x0804f220`, with **6/6 original relocations**. Its emitted
function is still **119 bytes**, so it remains diagnostic-only,
excluded from exact and selected manifests. Another 1,120 genuine
compiler recipes across scope-copy cleanup, iterator-copy, hash
constructor and fragment-list copy guards produced no original-length
improvement. Ledger remains **198 EXACT / 21 EXPERIMENTAL /
79 UNRESOLVED**. See
`vuc/docs/2026-10-14-scope-copy-75-byte-exact-prefix.md`.
After rebasing onto VCL `origin/main` at `62fdece`, a bounded GCC 3.3.3
source-lifetime and branch-probability investigation established an
original-length **72/306** partial for the previously unresolved
`TranslateFragList<MLGameString>` at `0x0806e5b0`. Both historical GCC
snapshots verify all **6/6** authentic relocations; the candidate retains
ordinary exception cleanup and compiler-generated `rep movsl` array copying.
It is **EXPERIMENTAL**, not byte-exact, and stays in `vuc/probes/` rather
than the accepted source or exact manifest. Separate historical controls
leave the shorter Scope-copy 75-byte prefix unresolved. The ledger is now
**198 EXACT / 22 EXPERIMENTAL / 78 UNRESOLVED** of 298 named functions,
with all **14,501** previously exact instruction bytes unchanged. See
`vuc/docs/2026-10-14-string-fraglist-relocation-partial-and-compiler-controls.md`.

Do not modify the root VCL `src/`, `config/`, `scripts/`, or progress history
for VUC-only experiments. Do not stage the untracked extracted upstream trees
or unrelated local files. Never change historical archives.

The latest VCL `main` rebase at `b95fc41` motivated a historically supported
exception-specification test in VUC's 513-bucket scope-copy template. A
genuine C++03 `MLHashTable::clear() throw()` declaration recovers the previously
119-byte, 75-byte-prefix `std::__uninitialized_copy_aux<Scope*, Scope*>`
as **120/120 byte-exact**, with all **6/6** original exception and constructor
relocations independently verified under both historical GCC 3.3.3 snapshots.
The declaration is isolated in `vuc/src/ScopeCopyTable.h` to preserve existing
exact headers; original common-header ownership is not established.
An additional bounded 192-build string-fragment loop sweep did not improve its
selected 72/306 experimental match. The ledger is now **199 EXACT /
22 EXPERIMENTAL / 77 UNRESOLVED** of 298 named symbols and **14,621**
accepted instruction bytes. See
`vuc/docs/2026-10-14-scope-uninitialized-copy-byte-exact.md`.

A follow-up source/RTL-boundary study checked **404** historical GCC
configurations without altering the accepted manifest. The unresolved
149-byte STL normal-iterator scope copy retains six authenticated call
relocations in a 136-byte natural instantiation; the original adds **9-byte
and 4-byte compiler-generated alignment gaps** before its return and
exception-handling regions. Neither a 256-build historical alignment matrix
nor 112 `_Construct`/`_Destroy` source-lifetime recipes emits the original
149-byte body. A separate 36-build test shows that annotating the
`MLGameString` copy constructor as non-throwing removes original exception
cleanup relocations in the 166-byte hash-table copy constructor. See
`vuc/docs/2026-10-14-iterator-eh-alignment-and-hashcopy-negative-batch.md`.

After incorporating `origin/main` updates `1eff5c3` and `42dca3b`, the
VUC string-fragment list at `0x0806e5b0` gained a stronger authenticated
original-length **75/306** experimental reconstruction (previously 72/306),
with **6/6 relocations** and its initial contiguous exact prefix improving
from **2 to 10 bytes**. Its ordinary C++03 local list index now advances
after fragment construction and immediately before appending; it is still
excluded from the exact manifest. Another 72 growth/copy-hint recipes and
70 historical hash-constructor index-alias controls did not improve their
respective valid original-sized candidates. The accepted inventory remains
**199 EXACT / 22 EXPERIMENTAL / 77 UNRESOLVED** and **14,621** exact code
bytes. See `vuc/docs/2026-10-14-string-fraglist-late-index-75-byte-partial.md`.

VUC's historical user guide restricts redistribution of modified compiler
versions. Treat reconstructed source distribution as unresolved until reviewed;
the root repository's MIT license does not automatically settle those rights.

The subsequent VUC reconstruction batch recovered the formerly unresolved
`MLHashTable<MLGameString, SymbolTableEntry*, ..., 513>` original 166-byte
copy constructor as a **genuine 80/166-byte original-length experimental
partial**, with **all four original relocation sites** independently
verified under both 2004-era GCC 3.3.3 snapshots. Its provisional explicit
`Node` constructor and saved next pointer are isolated in `vuc/probes/`,
not in accepted headers or the exact manifest. The original stack-owned
bucket index remains unmatched. Inventory: **200 EXACT / 24 EXPERIMENTAL /
74 UNRESOLVED**, with the same **14,930** accepted original instruction
bytes. See
`vuc/docs/2026-10-14-hash-copy-constructor-80-byte-partial.md`.

The latest batch, rebased on VCL `main` at `01d48ed`, recovered the originally
unresolved **`Value::operator||` at `0x0806f490` as 309/309 byte-exact**,
including its original `TypeConvert` relocation. Original ELF x87 branches
establish the important vector semantics: OR within each two-input float pair,
**AND across the four pairs**. Both historical GCC 3.3.3 snapshots emit all
309 original bytes from `vuc/src/ValueLogicalOr.cpp` under the selected
compiler controls. The same batch recovered an original-length, relocation-
authenticated **188/325 experimental `Value::operator&&`** partial; it remains
in `vuc/probes/`, not the exact manifest. Coverage advances to **200 EXACT /
23 EXPERIMENTAL / 75 UNRESOLVED** of 298, and **14,930** verified original
instruction bytes. Reproducible evidence and negative controls are in
`vuc/docs/2026-10-14-value-logical-or-byte-exact-and-partial.md`.

The next `vuc` investigation checked **475** additional ordinary GCC 3.3.3
source/compiler recipes without changing the accepted inventory or the
selected 75/306 `TranslateFragList<MLGameString>` partial. Among them,
an inlined `ASTNodeList::Add` `++length` form scores 85 overlapping bytes
but emits **311** rather than the original 306 bytes; combined source
lifetimes do not recover the required 306-byte form. The near-exact
`Print(ASTFunCall*)` remains **96/98** after 192 historical two-snapshot
register/source-owner trials, and the best printer object's altered build
scope regresses two already exact neighbors. Reproduction and acceptance
details: `vuc/docs/2026-10-14-add-ownership-and-printer-register-batch.md`.

The independent logical-AND De Morgan/float-lane batch tested **1,796**
ordinary C++03 source/compiler configurations. It improves the selected
**`Value::operator&&` experimental source from 190/325 to 192/325**
original-offset matching bytes, with authentic original length and its
`TypeConvert` relocation, independently verified under both historical
GCC snapshots. No new exact function was accepted. The separately
committed `be393e2` hash-table copy partial is preserved without changes;
the current 298-symbol inventory is **200 EXACT / 24 EXPERIMENTAL /
74 UNRESOLVED**. Evidence and reproduction:
`vuc/docs/2026-10-14-value-and-demorgan-192-byte-partial.md`.

The next `vuc` batch examined **515** further historical GCC 3.3.3
source/flag recipes across logical AND, Value division, and the hash-table
copy constructor. A larger authentic jump-alignment control improves the
original-length **`Value::operator&&` experimental partial from 188/325 to
190/325 matching original instruction bytes**, preserving its 115-byte exact
prefix and `TypeConvert` relocation under both historical GCC snapshots.
No additional whole byte-exact function was found; the accepted ledger stays
**200 EXACT / 23 EXPERIMENTAL / 75 UNRESOLVED**, with 14,930 exact function
bytes. Wrong-length variants and regressions were not promoted. Evidence:
`vuc/docs/2026-10-14-value-and-190-byte-partial-and-negative-controls.md`.

The October 15 VUC batch improved the **contiguous byte-exact entry prefix**
for the experimental `MLTempString` default **C2** constructor
(`0x08071c20`) from **22 to 37 bytes** by compiling its genuine
formatted-constructor predecessor in the historical source unit. Its total
strict match remains **95/101** bytes and all **9** relocations are verified
under both GCC 3.3.3 snapshots; this is **not** a new exact function.
A separate 671-source/compiler-recipe investigation retained the selected
`Value::ToString` **159/221** original-length partial and all its authentic
literal/call relocations. Accepted totals remain 200 exact functions, 24
experimental, and 74 unresolved. Reproduction and negative evidence:
`vuc/docs/2026-10-15-mltemp-c2-original-owner-37-byte-prefix.md`.

The October 15 historical-GCC vector arithmetic batch improves the selected
**`Value::operator+` experimental original-length partial from 172/219 to
203/219 identical original instruction bytes**, with **169 contiguous exact
entry bytes** and the authentic `TypeConvert` relocation independently
validated with both historical GCC 3.3.3 prerelease snapshots. The genuine
C++03 source updates four independent vector lanes in lexical order 3,1,2,0.
The accepted exact `Value` arithmetic sources remain untouched. Broad
source-level investigations of forced type conversion and the hash copy
constructor did not beat their existing selected partials. The exact
manifest remains 200 functions / 14,930 bytes; the original 298-symbol
ledger stays 200 EXACT / 24 EXPERIMENTAL / 74 UNRESOLVED. Complete evidence,
negative controls and reproduction steps:
`vuc/docs/2026-10-15-value-addition-203-byte-partial.md`.

The October 15 continuation reconstructed the previously unresolved original
**`Value::Set(ASTConstValExpr const*)`** as a new genuine-length,
relocation-authenticated **EXPERIMENTAL 67/455-byte partial** at `0x0806e7d0`.
The original AST layout (fields at +12..+24, string at +28), glibc internal
integer/float conversion calls, vector parsing and both Boolean literal
addresses are now represented in reproducible C++03 source. Both recovered
GCC 3.3.3 snapshots compile the saved candidate into **455 bytes**, with
**13 verified relocations and 11 verified original literal bytes**. This
does **not** establish behavioral equivalence or exact machine code. The
original 298-function ledger becomes **200 EXACT / 25 EXPERIMENTAL /
73 UNRESOLVED**, leaving the exact 14,930 byte total unchanged. The
2,760-variant compiler/source investigation also delimits additional
negative Value+ and Value/ strategies. See
`vuc/docs/2026-10-15-value-constval-455-byte-partial.md`.

The subsequent constant-parser reference audit verifies **13 distinct calls
plus two Boolean literal-address references in the original ELF**. The
selected original-length experimental reconstruction resolves 13 candidate
references because the compiler merges away two Boolean-fallback calls.
A separate *wrong-length* 459-byte diagnostic reconstructs all 15 original
reference sites and overlaps 213 original instruction offsets, with a
55-byte exact entry prefix; it is **not selected** as a strict partial.
The independent original-reference tests and follow-up evidence are in
`vuc/tests/test_value_constval_reference_sites.py` and
`vuc/docs/2026-10-15-vuc-set-15-reference-callgraph-audit.md`.

The October 15 accumulator control-flow reconstruction promoted the historical
77-byte `IsAccumulatorable(IR::Operator_Data*)` source from an experimental
**74/77** partial to a genuine **77/77 byte-exact function** with zero
relocations, independently reproduced by both recovered GCC 3.3.3 snapshots.
The key is a real C++03 negative guard and next-case label that recovers the
original final `jne`/`jmp` ordering, with no assembly or filler. The complete
ledger advances to **201 EXACT / 24 EXPERIMENTAL / 73 UNRESOLVED**, and strict
accepted code coverage is **15,007/15,007** bytes. The previously accepted
source and all initialized-data / BSS exact matches are unchanged. See
`vuc/docs/2026-10-15-accumulator-77-byte-exact-negative-goto.md`.

The October 15 follow-up established the first strict original-length
**`GetNextWord(char const*)` experimental source** at `0x080511e0`, moving
that symbol from UNRESOLVED to EXPERIMENTAL. The genuine C++03 byte-sized
whitespace/word-delimiter reconstruction compiles to the original **135
bytes**, matches **23/135** original-offset instruction bytes, and resolves
**5 actual relocation sites** targeting the original scanner globals
`currentPosition`, `wordPosition`, and `word`. Both recovered historical
GCC 3.3.3 snapshots reproduce the result. It is **not byte-exact** and
must remain under `vuc/probes/`, outside the exact manifest. The ledger
advances to **201 EXACT / 25 EXPERIMENTAL / 72 UNRESOLVED** out of 298,
while **15,007** fully exact function instruction bytes remain unchanged.
The same batch verified the original ELF's `__tcf_1` cleanup epilogue,
`Value::operator+` ascending x87 lanes and actual scalar branch alignment,
and STL iterator copy hidden return ABI, without forcing compiler registers
or fabricating padding. Nine genuine source/flag probes tested **2,784**
configurations; their negative and positive evidence is reproducible in
`vuc/docs/2026-10-15-getnextword-23-byte-partial-and-cfg-audit.md`.

The next October 15 source-level batch improved the selected experimental
`GetNextWord(char const*)` again from **23/135** to **50/135** original-offset
matching instruction bytes while preserving the original **135-byte** size,
all **5** authentic scanner-global relocation sites and period GCC 3.3.3
Jan25/Dec06 reproducibility. This uses natural signed-byte nested delimiter
comparisons and historical loop/label alignment, not patched instructions,
manual registers or alignment filler. The remaining register assignment
and code-layout differences prevent byte-exact acceptance; the candidate
stays in `vuc/probes/GetNextWordNestedRangePartialOct15f.cpp` and the
experimental manifest. The exact manifest is unchanged:
**201 EXACT / 25 EXPERIMENTAL / 72 UNRESOLVED** original functions,
**15,007 exact machine-code instruction bytes**. See
`vuc/docs/2026-10-15-getnextword-50-byte-partial-range-tree.md`
for source experiments, evidence and historical reproduction.

The following October 15 source-unit investigation promoted
`yyFlexLexer::yy_create_buffer(std::istream*, int)` at `0x080617b0`
from a **131/139** original-length experimental partial to **139/139
byte-exact** source. Inserting the genuine preceding `yyrestart()`
implementation into the same historical GCC 3.3.3 compilation unit
recovers all eight residual register-allocation bytes without manually
selecting registers. The reconstructed unit also independently reproduces
the original **93/93** `yyrestart()` body. Both historical snapshots
reproduce all **5** original relocation sites and **44** verified literal
bytes for `yy_create_buffer`. The exact ledger advances to **202 EXACT /
24 EXPERIMENTAL / 72 UNRESOLVED** of 298 original named functions, with
**15,146** exact instruction bytes. The whole linked ELF is still not
byte-identical. See `vuc/docs/2026-10-15-flex-create-original-prefix-exact.md`.

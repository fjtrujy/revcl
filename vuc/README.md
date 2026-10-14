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

Do not modify the root VCL `src/`, `config/`, `scripts/`, or progress history
for VUC-only experiments. Do not stage the untracked extracted upstream trees
or unrelated local files. Never change historical archives.

VUC's historical user guide restricts redistribution of modified compiler
versions. Treat reconstructed source distribution as unresolved until reviewed;
the root repository's MIT license does not automatically settle those rights.

# VUC: dumpPathKey main rebase and bounded compiler-context controls

Date: 2026-10-13. Branch `vuc`. Original VUC 2004.1 Linux
x86 ELF remains read-only in the tracked upstream ZIP. No root VCL
implementation files or historical archives were modified.

## Rebase

Fetched `origin/main` at **`8d609de`** (VCL
`dumpPathKey` reconstruction). The working branch started at
`8f49efc`, had no tracked modifications, and contained **146**
pre-existing unrelated untracked paths. Local `main` now tracks
`origin/main`. **104** existing VUC commits replayed cleanly;
rebased starting head **`19d36e8`**. No untracked collisions,
conflicts, deleted files or unrelated updates occurred.

The new main evidence is
`config/matching-evidence/dumppathkey-deferred-owner.md`:
a genuine ordinary inline free function in a `#pragma interface`
header can inherit the compiler pass state of its
`#pragma implementation` owner, depending on *real function
definition/emission order*. This changed a GCC 3.2.2 peephole2
dead-register assignment and produced a new exact 173/173
VCL function while preserving its independent owners.

For VUC, this is a **source-context hypothesis**, not proof of
original translation-unit layout or a license to add dummy compiler
state. VUC's historical compiler is a different GCC 3.3.3 prerelease.
The newly tested variants therefore use only real VUC source methods,
ordinary C++ constructs and the two recovered GCC dates.

## Experimental observations; no new exact function

**No new exact or improved original-size partial was produced.**
The accepted manifest, exact source files and data declarations are
unchanged. Negative controls are preserved so that future rounds do
not repeat the same search.

### Instruction arena destructor: 45/46 remains

Original `__tcf_1` at `0x08051980`, **46** bytes.
The earlier real `InstructionList`/arena compilation unit generated
**45/46**, differing at instruction byte offset **42**
(`pop %eax` versus the original `pop %ecx`).

`vuc/probes/evaluate_instruction_genuine_method_order_oct13.py`
reorders actual `InstructionList::Add`, `AddL`, `GenLabel`,
`GenTmp`, `Print`, and
`Instruction::ToString`, `operator new`, and real static
`instrData` definition while retaining the real arena lifetime
definition at the end. It covers a baseline, all cyclic rotations,
neighbor swaps and representative method groups.

All original-size results remain **45/46** with the same offset 42.
No fabricated unrelated methods, register constraints, assembly
snippets, explicit NOP padding or machine-byte edits are used.
The VCL's deferred-inline owner effect is therefore **not**
established for this VUC destructor by genuine method order alone.

### Flex next buffer: alignment hypothesis rejected

Original `yyFlexLexer::yy_get_next_buffer()` at
`0x080611c0`, **550** bytes. The accepted experimental
`vuc/probes/yy_get_next_buffer_partial_oct13.cpp`
remains **430/550**, with the original `realloc` target and
three genuine scanner diagnostics resolved. Its literal evidence
retains the previously documented source-unit padding limitation;
it is not promoted.

The original disassembly places copy-loop and buffer-growth
entries at 16-byte-relative offsets `+0x60` and `+0x160`.
The partial's early conditional block instead targets `+0x4b`
and its growth loop starts at `+0x15c`, which motivates testing
actual compiler jump/label/loop alignment rather than emitting
synthetic pad instructions.

`vuc/probes/evaluate_yy_next_target_alignment_oct13.py`
tests **64** genuine GCC alignment combinations
(`-falign-jumps`, `-falign-labels`, `-falign-loops`
in `{1,4,8,16}`). The only matching 550-byte original-size
configuration remains **430/550**; most other alignments inflate
the compiled function well beyond 550 bytes (up to 724 in tested
combinations). The original NOP pattern cannot be attributed to
one global alignment flag alone.

### AST statement list: 119-byte length not recovered

Original `Translate(ASTNodeList<ASTStmt*>&)` at
`0x0806ada0`, **119** bytes.
`vuc/probes/evaluate_translate_stmt_optimizer_oct13.py`
tests a genuine `IR::Seq_Data` construction source against
both December 2003 and January 2004 GCC 3.3.3 builds,
with/without `-frename-registers` and 21 optimizer variants.
None emits the original 119-byte method. Typical lengths
are 116, 130, 132 and 133. Consequently no strict
original-size byte score is asserted.

The attempt directly tests main's scoped optimizer lesson
(e.g., `-fno-gcse`) against VUC, but no VUC-specific flag
exception is justified or retained.

## Next evidence-based experiments

The most informative follow-up is an actual **GCC 3.3.3 RTL
peephole2/postreload inspection**, comparing the scratch
register choice for the one-byte destructor with a genuine
single-owner source layout. For the Flex buffer function,
reconstruct the original buffer-growth CFG and pointer
lifetimes before further compile-flag sweeps. For the AST
statement list, recover the authentic lifetime of the
`IR::Stmt_Data*` result and its inline `IR::Seq_Data`
constructor, rather than trying additional equivalent
branch shapes blindly.

The strict VUC inventory remains **195 EXACT, 15
EXPERIMENTAL, 88 UNRESOLVED** out of 298 original
functions (13,992 accepted instruction bytes), pending
regression confirmation. Original initialized data includes
244 named objects / 71,536 bytes, and there are 12 BSS
declarations. Full linked ELF byte identity is not yet
established.

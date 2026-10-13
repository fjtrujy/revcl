# VUC bounded batch: historical GCC RTL and genuine source context

Date: 2026-10-13. Branch: `vuc`. Starting HEAD:
`2d06468` on local `main` base `8d609de`.
The original target remains the archived VUC 2004.1 Linux ELF.

## Outcome

**No newly byte-exact function or improved original-length partial was
recovered.** The exact acceptance manifest is unchanged. The purpose of
this batch is to make the one-byte generated destructor discrepancy
actionable at the actual compiler-pass level, and test two independent
promising functions against the original compiler and relocated bytes.

No old archives, exact source fragments, host project VCL sources,
linker binaries, compiler binaries or already-accepted manifests were
modified. All experiments compile genuine ordinary C++03 source
without assembler, artificial instructions, fabricated addresses or
manual register constraints.

## Instruction arena destructor: GCC RTL stage isolated

Original `__tcf_1` at **`0x08051980`**, length **46 bytes**,
finishes with `pop %ecx; pop %ebx; pop %ebp; ret`. The
original-length C++ source in
`vuc/probes/instruction_list_arena_unit_oct14.cpp` finishes with
`pop %ebx; pop %ebx; pop %ebp; ret`. The first pop is a
**dead stack-discard destination**, not an incorrect restored value.
The relocation-aware checker reports **45/46** matching instruction
bytes, only offset **42** differing, and authenticates both the
`Instruction::instrData` BSS reference and `operator delete`
relocations.

`vuc/probes/evaluate_instruction_tcf_rtl_next.py` recompiles the
complete genuine `InstructionList` and arena methods using
the original GNU as 2.15 wrapper with
`-O2 -frename-registers -fuse-cxa-atexit -da`, separately for
the recovered December 6, 2003 and January 25, 2004 GCC 3.3.3
snapshots. Its measurements establish the same **45/46** result
and generated `EBX` discard with *both* compilers.

The GCC RTL `26.flow2`, `27.peephole2`,
`28.rnreg` and `30.sched2` dump sections for `__tcf_1`
show the epilogue register choice preserved to final scheduling.
`flow2` already has the callee-save EBX restore;
`peephole2` materializes a second, dead EBX destination for the
stack adjustment before it. Register renaming and scheduling retain
that implementation rather than producing the original ECX scratch.
The RTL dumps establish the *candidate compiler behavior*, not a
recovered historical original RTL or an exact original compiler pass
trace. Their files are deliberately kept outside Git in
`/tmp/vuc-binutils215/instruction-rtl-reproduction-next/`.

For the next attack, inspecting the true ownership/translation-unit
state **before** flow2 and peephole2 is justified; editing the
C++ loop to directly request ECX or replacing the pop instruction
would defeat the source-reconstruction objective. Earlier genuine
method reorder and deferred-header experiments also remained 45/46.

## AST function-call printer: 96/98, no new exact source

Original `Print(ASTFunCall*, std::ostream&)` at
`0x0805b770`, length **98**, has seven supported relocation
sites and three original printer literals.

`vuc/probes/evaluate_print_ast_real_methods_next.py` tests
**11** legitimate definition arrangements of the real
`Print(ASTFunCall*)`, `Printer_UglyPrint`, and the recovered
`Print(ASTNodeList<ASTExpr/Stmt/Decl*>)` template definitions
and instantiations, each with four period GCC optimization variants.
The intact printer, with `-O2` and no explicit
`-frename-registers`, still produces **96/98** with the
same offsets **23 and 29** (compiler chooses EAX for callee
load whereas the original chooses ECX). Adding
`-frename-registers` instead leaves **92/98**.
No arrangement changes the two mismatching original bytes.

As an independent check of the recent VCL `main` deferred-owner
technique, `vuc/probes/evaluate_printer_inline_owner_next.py`
builds the genuine printer as an `inline` free function in
period-supported normal or `#pragma interface`/
`#pragma implementation` headers. With
`-fkeep-inline-functions` the function is emitted at original
length 98, but libstdc++ contributes additional iconv diagnostic
literals **ahead of** the genuine function's strings. The
relocation-literal verifier correctly rejects the object:
**neither the literal ownership nor original source-unit placement
is authenticated**, so there is no claim of a byte match from
these variants. No dummy caller was added to force emission.

## IR statement-list translator: original length not recovered

Original `Translate(ASTNodeList<ASTStmt*>&)` at
`0x0806ada0`, length **119**, constructs 16-byte polymorphic
`IR::Seq_Data` nodes.

`vuc/probes/evaluate_translate_seq_ctor_layout_next.py`
tests **nine** ordinary C++ polymorphic constructor/field
initialization and inheritance layouts, **five** genuine indexed,
while, continue and inverted-branch loops, and **four** historical
optimizer combinations (180 builds). The closest natural
implementations still generate **116 bytes**, not the original
119. None has an original-size / relocation-authenticated byte
result. These source forms are negative experiments only, not
a replacement for the existing experimental translation source.

## Reproduction

The canonical machine/compiler and original ZIP are configured
on the user's existing `revcl-oracle` VM.

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_instruction_tcf_rtl_next.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_print_ast_real_methods_next.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_printer_inline_owner_next.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_translate_seq_ctor_layout_next.py"
python3 -m unittest discover -s vuc/tests -q
```

The exact manifest remains **195** named function symbols,
**13,992** independently verified original instruction bytes.
The named-function inventory is **195 EXACT, 15 EXPERIMENTAL,
88 UNRESOLVED**, out of 298. The 244 original initialized-data
objects (71,536 bytes) and 12 BSS declarations remain separate
strict gates. Whole linked ELF identity is **not established**.

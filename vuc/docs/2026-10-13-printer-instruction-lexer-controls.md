# VUC source-unit near-matches: printer, arena destructor, and lexer buffer state

Date: 2026-10-13. Branch: `vuc`.

These are **non-exact** ordinary C++ experiments preserved to reduce
repeated register/optimizer guessing. No original ELF, archived ZIP,
accepted manifest, or code-generation toolchain was modified.

## `Print(ASTFunCall*, std::ostream&)`: original 98 bytes

The original at `0x0805b770` calls `Print(ASTLValExpr*)`, the stream
literal inserter and the AST-expression-list printer. The three
literal strings `"("`, `", "`, and `")"` are authenticated through
original ELF section addresses. A source unit including the genuine
following `Printer_UglyPrint()` and instantiated AST list printers
is compiled by historical GCC 3.3.3 in
`vuc/probes/evaluate_printer_context_oct13.py`.

The existing 98-byte natural C++ body compiles to **96/98** matching
instruction bytes without `-frename-registers` (offsets 23 and 29
choose EAX rather than the original ECX). With register renaming, the
combined source makes **92/98** bytes match, but the previously accepted
module printer remains **40/40** and its global wrapper **31/31** under
the accepted compiler flags. Eighteen source/context/optimizer
combinations involving genuine list-printing instantiations failed to
produce a fully exact function. The new diagnostic config
`vuc/config/print_ast_fun_context_oct14_probe.json` maps all **nine
literal bytes** from the same real source object; it is **not** added
to the exact manifest.

## `Instruction` arena destructor callback: original 46 bytes

At `0x08051980`, generated `__tcf_1` has the correct exception-free
cleanup loop, identical 46-byte length and all three relocations
resolved, but the candidate epilogue uses `pop %eax` while the
original uses `pop %ecx` at offset 42. The evaluator
`vuc/probes/evaluate_instruction_unit_order_oct13.py` tests all six
real definition orders for `Instruction::ToString()`, the allocator,
and the arena lifecycle under six historically supported flag sets.
All normal variants remain **45/46**; disabling GCC's second scheduler
gives worse results.

A further source fragment in
`vuc/probes/instruction_list_arena_unit_oct14.cpp` integrates real
`InstructionList::{Add,AddL,GenLabel,GenTmp,Print}` with the original
`Instruction::ToString`, allocator and arena lifecycle. Across ordinary
GCC register/branch/section flags, the destructor still matches only
**45/46**. This rules out the *tested* genuine-method context; it does
not justify changing the register by inline assembly, artificial
padding, invented functions or raw machine-code copying. The accepted
separate InstructionList and arena-helper functions remain unchanged.

## `yyFlexLexer::yy_load_buffer_state()`: original 39 bytes

`vuc/probes/flexlexer_load_original_unit_oct13.cpp` adds the natural
buffer-state load sequence to the genuine recovered FlexLexer source
unit, preserving original field widths and byte offsets. The original
loads active buffer, count, input stream and cursor, and writes the
current buffer/count/cursor/hold-byte fields. GCC 3.3.3 emits an
original-length **39-byte** function, but its register allocation and
store scheduling differ; the best of five tested compiler combinations
is **25/39**. The unaccepted probe is reproducible with
`vuc/config/yy_load_full_unit_oct14_probe.json`.

## Additional hash-table source evidence

`vuc/probes/evaluate_symboltable_enter_oct13.py` tests 96 natural
source/index/insertion/flag combinations of `SymbolTable::Enter()`
against the original **185-byte** body at `0x0806a330`. The recovered
vector-indexed scope form correctly reproduces the `2052`-byte
per-scope stride and five named C++/runtime calls. The candidate
compiles to **194 bytes** under default `-O2 -frename-registers`,
**180 bytes** with jump alignment 1, and **186 bytes** with jump
alignment 8. Its allocation, exception landing pad, and two insertion
stores remain insufficiently reconstructed. The original-function
partial config is tracked as
`vuc/config/symboltable_enter_oct13_probe.json`; it is not accepted.

## Reproduction

Run each script inside the existing `revcl-oracle` VM with the recovered
`/tmp/vuc-binutils215/g++-jan25-as215` wrapper:

```sh
r=/path/to/revcl/vuc
python3 "$r/probes/evaluate_printer_context_oct13.py"
python3 "$r/probes/evaluate_instruction_unit_order_oct13.py"
python3 "$r/probes/evaluate_symboltable_enter_oct13.py"
```

None of these experiments changes the 175 accepted exact function
records or claims whole-ELF byte equality. Future work should seek
new provenance from authentic original translation-unit content,
libstdc++ headers, GCC 3.3 RTL pass differences, and original literals,
not retry ungrounded brute-force flag sweeps.

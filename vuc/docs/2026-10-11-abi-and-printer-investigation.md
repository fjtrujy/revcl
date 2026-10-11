# VUC October 11 ABI and printer source probes

These investigations preserve source-derived, **non-exact** hypotheses only.
All accepted results are independently checked in the separate exact-function
manifest. The complete original ELF is still not byte-identical.

## MLTempString::Set() return-value hypothesis

Original `MLTempString::Set(char const*, ...)` at `0x08071d00` has
**117 instruction bytes**. Its tail explicitly writes the preserved
`this` pointer to `EAX` before the epilogue, while the original
C++ symbol name does not encode the return type.

The existing provisional declaration `void Set(...)` emits **118 bytes**
with the January GCC 3.3.3 code generator, and is unlikely to capture
the original ABI. A source-level candidate that returns `this`
(`vuc/probes/ml_temp_set_return.cpp`) emits **117 bytes** with
`-O2`, reproducing the original two callee-saved registers and
**111/117** original instruction bytes, with eight original ELF
relocations resolved. The remaining six bytes concern register
allocation around the format argument and the singleton's
`memset` initialization. The original source could have returned a
pointer or reference; both share the same i386 calling convention.
This is a **hypothesis**, not a proven signature.

`evaluate_ml_temp_return_flags.py` tests period-plausible compiler
switches. The historical December 2003 GCC 3.3.3 and January 2004
GCC 3.3.3 both produce the same best result. The accepted shared
header and code were not modified; a partially matched return-type
guess is not sufficient for promotion.

## Print(ASTFunCall*) register allocation

The original `Print(ASTFunCall*,std::ostream&)` at `0x0805b770`
is **98 bytes**. The source hypothesis models the original callee
pointer at offset 8, argument-list object at offset 12 and the
original three string literals `"("`, `", "`, `")"`.
Historical GCC with `-O2` matches **96/98 bytes**, with all seven
relocations and seven literal bytes verified. Both differences are
the register chosen for the initial callee pointer (EAX rather than
the original ECX). The `-frename-registers` variant matches
**92/98** bytes; neither is acceptable.

`evaluate_ast_fun_call.py` explores ordinary C++ expression shapes;
`evaluate_ast_fun_call_flags.py` explores period compiler switches.
Both original-date compiler builds emit the same non-exact register
selection for the best `-O2` hypothesis. No assembly or padding
has been introduced to force a superficial match.

## Other unresolved leads

- `Canonize2_Reduce(IR::Module_Data*)` reaches 83/85 bytes with
  original jump alignment, but differs in the register used by the
  initial module-status test. `evaluate_canonize_field.py` checks
  ten plausible 32-bit status field declarations; no full match.
- `script_error(char const*)` has a provisional ordinary-C++
  `std::cout` formatting and error-flag hypothesis. The initial
  source emits 158 bytes versus 161 original; the original global
  file-name/line-number operands and stream targets are identified,
  but the emitted function is **not** fully verified.
- The `MLGameStringTable::Get()` singleton and the original
  `MLTempStringData::VFormatString()` remain non-exact, as documented
  previously. The current branch retains their probes.

## Preserved acceptance baseline

As of commit `68c8dd6`, the strict gate is **147/147 function
records, 8,325/8,325 instruction bytes, 370 resolved relocations,
132 verified literal bytes**. All 60 host tests pass.

None of the investigations in this note is included in the accepted
manifest, and no whole-executable match percentage is asserted.

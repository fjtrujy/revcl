# VUC short-function negative controls after the Value startup milestone

Date: 2026-10-13; branch `vuc`.

These source-level experiments are preserved because they rule out
several plausible source-neighbor/optimization hypotheses. They are
NOT exact-function acceptances and the canonical original ELF was
never altered.

## `AsmOutput_WritePrologue` — 22/30 remains non-exact

Original `0x08049b40` is 30 instruction bytes. The implementation
writes the original string ending in `--endenter`, which occupies
92 bytes including the terminator. The immediately following genuine
`AsmOutput_WriteEpilogue()` outputs the original 19-byte string ending
in `--endexit`. Both are recreated in ordinary C++ in
`vuc/probes/asm_output_real_unit_oct13.cpp`.

`vuc/config/asm_output_unit_oct13_probe.json` checks the original
ostream-insertion relocation and **all 111 source-literal bytes**,
mapped to the original addresses `0x080720a0` and `0x080721c4`.
Under the recovered original-era GCC 3.3.3 compiler, the prologue
still matches only **22/30** instruction bytes. The compiler loads
the literal into EDX where the original uses EAX and thus schedules
the outgoing arguments differently.

`vuc/probes/evaluate_asm_unit_context_oct13.py` tests thirty bounded
combinations of the genuine neighboring epilogue, `constantsTabStop`
global/iostream initializer and accepted `AlignConstant()` method,
under ordinary GCC alignment and function-section options. None
produces a full prologue match. The existing independently accepted
30/30 epilogue is unchanged. Do not solve this by injecting register
moves or artificial predecessor code.

## `IsAccumulatorable` — 61/77 with authentic predecessor

Original `IsAccumulatorable(IR::Operator_Data*)` is 77 bytes at
`0x0805a570`. The reconstructed `GetOpType(IR::Expr_Data*)`, a
previously accepted 117-byte original function at `0x0805a4f0`,
immediately precedes it in the original ELF. The natural C++ source
unit `vuc/probes/accumulatorable_genuine_context_oct13.cpp` includes
both definitions, using the authenticated pointer/type-offset layout.
The original-length variant under `-O2 -falign-jumps=1` still matches
**61/77**, with misaligned branch/control-flow blocks rather than the
same kind of register-renaming mismatch that the Value and Canonize2
milestones resolved. Using default jump alignment produces **91 bytes**
instead of the original 77. This predecessor does not resolve the
problem; revisit with new evidence for original branch polarity and
alignment decisions.

The reconstructed source units are experimental hypotheses, not
complete historical source files. They do not change any entries in
`vuc/config/exact_candidates.json` or the function-coverage ledger.

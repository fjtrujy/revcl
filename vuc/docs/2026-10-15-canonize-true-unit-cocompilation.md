# VUC Canonize.cpp: five newly recovered methods compiled as one C++03 unit

Date: 2026-10-15. The original ELF VUC 2004.1 Linux i386 and
historical GCC 3.3.3/assembler 2.15 remain the authority.

This batch shifted away from exhausted Scope/Type hypotheses and
reconstructed five previously unresolved VUC functions. To
investigate *actual source-unit provenance* rather than another
generic compiler-flag sweep, the new
`vuc/probes/evaluate_canonize_real_unit_oct15p.py` assembles a
single historically plausible `Canonize.cpp` from their natural
C++03 source definitions:

1. `Canonize_TryReduceExpr<unsigned int>`, ELF `0x0804ffb0`
2. `Canonize_TryReduceExpr<int>`, ELF `0x08050120`
3. `ReduceBinOp<unsigned int>`, ELF `0x08050540`
4. `ReduceBinOp<int>`, ELF `0x08050680`
5. `ConcatenateBinOp`, ELF `0x080507c0`

The `Canonize_FoldExpr` application function at `0x080502c0`
lies between the first two template bodies and the ReduceBinOp
specializations in the original executable. Its 410-byte body is
not reconstructed yet, so this is still an incomplete source unit.
Original symbol order is respected in the primary control; a
second placement with ReduceBinOp definitions before Canonize is
included explicitly to measure legitimate predecessor effects.

The original template type declarations, parameter ABI and
source-level names are preserved, including the genuine
`MLHashTable<MLGameString,MLGameString>::FindPtr`,
`localEnv` and `globalEnv` named original globals.
The same source passes both historical GCC snapshots. No
assembly byte manipulation, invented relocations, linker layout
faking or dummy padding was used.

## Strict original-offset scores from the same compiler object

| Source order and flags | Canonize unsigned / signed | Reduce unsigned / signed | Concatenate |
| --- | --- | --- | --- |
| Original order, -O2 -frename-registers -falign-jumps=4 -falign-labels=2 -falign-loops=1 | **116/359**, **116/359** | 299/315 size rejected | **223/346** |
| Original order, -O2 -frename-registers -falign-jumps=8 | 373/359 size rejected | **294/315**, **300/315** | 354/346 size rejected |
| Reduce definitions before Canonize, j4/labels2 | **116/359**, **116/359** | 299/315 size rejected | **223/346** |
| Reduce definitions before Canonize, j8 | 373/359 size rejected | **300/315**, **300/315** | 354/346 size rejected |

These results reproduce **identically with the recovered Jan25 and
Dec06 GCC 3.3.3 compiler snapshots**.

In the original-order source-unit reconstruction, adding actual
Canonize predecessors changes the register allocation for
`ReduceBinOp<unsigned>`, losing **six previously matching
instruction bytes** at the original 315-byte length. Moving
ReduceBinOp definitions earlier restores 300/315. The signed
specialization is unaffected. This is evidence of *real source-unit
compiler ownership effects*, analogous in principle to the earlier
FlexLexer `yyrestart` predecessor discovery. It does not
justify changing the historical source order merely to improve
a candidate's score.

When j4 alignment makes both Canonize and Concatenate have original
sizes, ReduceBinOp becomes shorter (299 instead of 315). This is
further evidence that authentic missing source-unit predecessors,
source shapes and compiler block alignment must be understood before
claiming a full historical compilation unit or exact output.

Generated C++03 sources, ELF32 objects, and full original-size,
relocation-aware JSON scores are kept outside Git under
`/tmp/vuc-binutils215/canonize-real-unit-oct15p/` in the
existing Lima guest. Reproduce with:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_canonize_real_unit_oct15p.py
```

## Next original function

The untouched 410-byte `Canonize_FoldExpr(IR::Expr_Data*)`
original at `0x080502c0` recursively simplifies binary
expressions and may replace constant trees with 12-byte
MLGameString-backed expression objects and 16-byte unary
wrappers. Its ELF disassembly confirms genuine calls to
`IsFoldable`, `ConcatenateBinOp`,
`MLGameString` copy constructor, `operator new`,
`operator delete`, and `_Unwind_Resume`.
It also emits a diagnostic through the original `std::ostream`
if the expression kind is not 6. Reconstructing this true
intermediate predecessor is more promising than continuing
broad alignment permutations.

The accepted inventory remains **204 EXACT / 32 EXPERIMENTAL /
62 UNRESOLVED**. Original whole-ELF equality remains unproven.

# VUC ConcatenateBinOp after the ReduceBinOp exact breakthrough

**Date:** 2026-10-23. Branch `vuc`. Immutable 2004.1 original ELF.

## Question

Can the successful GCC 3.3.3 `__builtin_expect(opcode==10,0)`
source-level dependency that made both ReduceBinOp instantiations
**315/315 exact** also improve their adjacent string-serializing
`ConcatenateBinOp(IR::Operator_Data*,MLGameString const&,...)`?

The accepted experimental ConcatenateBinOp function at `0x080507c0`
is **223/346** original-offset bytes, including **20 authentic
relocations** and **44 independently authenticated source literal
bytes**. Baseline compiler flags stay
`-O2 -frename-registers -falign-jumps=4 -falign-labels=2
-falign-loops=1`.

## Source/provenance-preserving controls

`vuc/probes/evaluate_concat_binop_original_branch_prediction_oct23.py`
varies exactly one of the real 14 original opcode predicates at a
time (`__builtin_expect(guard,0)` or `1`). The operator values
and strings, result constructor, GNU C++03 MLString calls and
all observable operations are untouched.
**28 source candidates × 2 GCC3.3.3 snapshots = 56 builds**.
The entire compiler-object, source and score ledger is retained
outside Git in the existing VM at
`/tmp/vuc-concat-opcode-prediction-oct23/`.

A new practical complication: annotating a branch changes GCC's
order of the **real 16 emitted original operator strings** in
`.rodata`. The usual original-offset manifest correctly rejects
those builds, because its literal offsets describe the baseline
compiler order. It would be erroneous to call that a code mismatch.

`vuc/probes/evaluate_concat_binop_authentic_literal_reorder_oct23.py`
therefore reads the *actual original ELF*, authenticates the bytes
and named original addresses for all 16 existing string ranges,
then remaps each unique entire NUL-terminated source literal in a
candidate's object section to that **specific previously known
original address**. Missing, extra, ambiguous, truncated or otherwise
unrecognized literal strings cause rejection; no original address,
source string or byte is invented. The untouched strict
`match_probe.run` verifier checks every reassembled code
relocation and all 44 source-literal bytes after this mapping.

**Result:** none of the 28 combinations improves beyond **223/346**
with full 346-byte length, 20/20 relocations, and 44/44 literals.
For example, the exact-winning ReduceBinOp guard at opcode 10,
predicted unlikely, remains **223/346** here with both recovered
GCC3.3.3 snapshots. Other variants are shorter/longer or
original-length with fewer code bytes. This negative result is
specific to the genuine ConcatenateBinOp source; it does not
contradict the two newly exact ReduceBinOp specializations.

The selected `ConcatenateBinOp` source and partial checkpoint floor
remain unchanged. No changes to `vuc/src` or exact acceptance arise
from these negative controls; the source-literal provenance helper
may be reused for other authentic source-unit string-order studies.

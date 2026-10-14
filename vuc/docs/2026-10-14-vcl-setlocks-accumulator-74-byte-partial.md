# VUC: GCC 3.3.3 paired-prediction reconstruction after VCL setLocks

Date: 2026-10-14. Active branch: `vuc`.

## Repository checkpoint

Fetched and rebased the current VUC branch on **`origin/main` at
`1693559`** (`Complete setLocks.cpp matching at 0x0807d458`).
The rebase replayed **117 commits** without conflicts. The previous 163
untracked paths were preserved, including user work and extracted upstream
archives; none was reset, staged, overwritten or removed. The rebased
checkpoint was `f088d08` before this batch's changes.

The new VCL evidence in
`config/matching-evidence/reduce-extents-reconstruction.md` and
`docs/matching-notes.md` established that two independent,
semantically-neutral GCC **3.2.2** `__builtin_expect` annotations can
jointly change a single code-alignment request without changing useful
instructions. This suggested checking the same mechanism against VUC's
different **GCC 3.3.3 (January 25, 2004 prerelease)** with GNU as 2.15.
VCL flags, function offsets, reconstructed source, historical-authorship
assumptions and compiler-version conclusions are **not** transferable as
facts about the VUC binary.

## New, independently verified improvement: IsAccumulatorable 74/77

Original: `IsAccumulatorable(IR::Operator_Data*)` at `0x0805a570`,
**77 original instruction bytes**, no relocation operands.

Prior selected experimental source with `-falign-jumps=1` achieved
**61/77**. The improved source is
`vuc/probes/ir_accumulatorable_joint_oct14.cpp`. The updated
`vuc/config/ir_accumulatorable_probe.json` uses genuine C++ source
compiled by the original-era compiler with
`-O2 -frename-registers -falign-jumps=8`.

Its four annotations are attached to real control-flow decisions:

```cpp
if (__builtin_expect(op->kind == 11, 0)) {
    if (__builtin_expect(op->operand->kind == 3, 1)) return true;
}
if (__builtin_expect(op->kind == 13, 0)) {
    if (op->operand->kind == 3) return true;
}
if (op->kind == 12) {
    if (__builtin_expect(op->operand->kind == 3, 0)) return true;
}
return false;
```

The original 77-byte function has a four-byte assembler-generated NOP
at relative `+0x1c`, a seven-byte NOP at `+0x29`, case-12 at
`+0x20`, success at `+0x30`, case-13 at `+0x37` and case-11
at `+0x42`. The recovered candidate reproduces all those blocks,
registers, comparisons and jump destinations through offset
**`+0x48`**.

The strict VUC ELF matcher reports **74/77** matching instruction
bytes. The only differences are `+0x49`, `+0x4a`, `+0x4c`,
corresponding to equivalent but opposite final branch encodings:

| Position | Original 2004 ELF | Current genuine GCC candidate |
| --- | --- | --- |
| `+0x49` | `jne` (false, to case-13) | `je` (true, to success) |
| `+0x4b` | unconditional jump to success | unconditional jump to case-13 |

The function behavior is equivalent for the three operator kinds and
their operand tag, but opcode/displacement bytes differ. This is an
**improved experimental candidate, not an exact-match acceptance**.
It stays in the existing experimental manifest and is **absent from
`vuc/config/exact_candidates.json`**. The compiler annotations
reconstruct one plausible CFG/layout; **their original-author use
has not been established**. This partial improvement alone does not
establish original source, original full translation-unit ownership,
or whole-ELF byte identity.

## Negative controls performed in this batch

The following reproducible scripts compile and check actual C++03
functions, not emitted assembly, manually forced bytes, padding
declarations or fake symbol relocation addresses:

- `evaluate_accumulator_joint_predictions_oct14.py` — 81
  combinations of the three real outer conditions and a shared operand
  decision under six compiler/branch-alignment settings: **486
  compilation recipes**. The `-falign-jumps=1` candidate remains
  **61/77**, and the `-falign-jumps=8` branch candidates show that
  aligned positions alone are not enough.
- `evaluate_accumulator_independent_guards_oct14.py` — all
  `3^6=729` independently plain/likely/unlikely conditions (three
  operator branches and three operand branches), using the genuine
  GCC 3.3.3 `-falign-jumps=8` code-generation option. Three
  original-length combinations survive; the best is **74/77**.
- `evaluate_accumulator_last_edge_oct14.py` — 23 equivalent C++03
  case-11 guard forms under 16 period compiler choices:
  **368** further controls. These include inverted guards,
  genuine nested `if`/`else`, `goto`, typed locals, integer
  arithmetic comparisons, original-era scheduling, branch cost,
  register moves and block-reordering options. None corrects the
  remaining branch orientation while preserving the first 74 bytes.
- `evaluate_accumulator_switch_case_oct14.py` — all six source
  `switch` case orderings, four genuine arm/return topologies,
  `3^3` operand branch-likelihood combinations, and two period
  alignment choices: **1,296** compiled controls. None emits the
  original 77-byte body under this model.
- `evaluate_arena_likely_destructor_oct14.py` — 14 genuine
  `Instruction` cleanup-loop and null-test variants under six
  historical compiler controls: **84** configurations. The
  destructor `__tcf_1` stays at **45/46**, with the same dead
  cleanup-pop register difference at relative **+42**.

In total the bounded batch attempts **2,963 historical GCC recipes**
(including rejected sizes and compiler errors). A matching function
length alone never triggers acceptance. The selected partial is
recompiled independently from its committed source and verified by
`vuc/scripts/match_probe.py`.

## Run again

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
  -falign-jumps=8 -c "$R/probes/ir_accumulatorable_joint_oct14.cpp" \
  -o /tmp/vuc-binutils215/accumulator-joint-repeat.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/accumulator-joint-repeat.o \
  --config "$R/config/ir_accumulatorable_probe.json"
```

The source produces `length:77`, `exact_bytes:74`,
`different_offsets:[73,74,76]`, `exact:false`. All generated
C++ source variants and ELF objects reside only under
`/tmp/vuc-binutils215` inside the existing Lima VM.

## Status and further hypotheses

The 298 named VUC function records remain **197 EXACT,
14 EXPERIMENTAL and 87 UNRESOLVED**. There is **one independently
improved partial**, and **no new exact function** in this batch.
Preserving the original 197 accepted records is mandatory.

All independent regression gates passed *after* updating the
experimental source/manifest: **197/197** previously exact functions
and **14,417/14,417** instruction bytes; **244/244** original
initialized-data objects and **71,536/71,536** original data bytes;
**12/12** BSS declarations and 18,437 source-allocation bytes; and
**149/149** host tests. The intentional negative acceptance fixture
prints `Candidate byte-exact acceptance: FAIL` as part of a passing
test; it does not indicate a regression. The new 74/77 candidate was
separately recompiled with its updated checked manifest. No complete
linked VUC ELF identity is claimed.

The highest-value next attempt is a genuine source-level explanation
of the case-11 cold fallthrough/conditional emission, potentially
through a real caller or a more accurate original function/source-unit
boundary. Isolated equality guards and simple `__builtin_expect`
polarity are exhausted by this batch. The next unworked application
family remains the original `Canonize_Reduce` 224-byte hash/IR
dispatcher, where typed fragments and real scope ownership matter more
than flag sweeps.

Full original-ELF byte identity has **not** been achieved.

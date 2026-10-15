# VUC IsAccumulatorable — first new exact function from C++03 inverted guard

Date: **2026-10-15**. Work in existing `/path/to/revcl`
branch `vuc`. Original: unchanged archived VUC 2004.1 Linux i386 ELF.
Historical compiler snapshots: GCC 3.3.3 January 25, 2004 and
December 6, 2003, both assembled with GNU binutils 2.15.

## Original evidence and new exact byte-for-byte source

| Attribute | Result |
| --- | --- |
| Original name | `IsAccumulatorable(IR::Operator_Data*)` |
| Mangled symbol | `_Z17IsAccumulatorablePN2IR13Operator_DataE` |
| Original symbol location | `0x0805a570` |
| Original function size | **77 bytes** |
| Previous partial | **74/77** same-length bytes |
| New accepted source | `vuc/src/IRAccumulatorableExact.cpp` |
| New byte-exact score | **77/77 original instruction bytes** |
| Original relocation sites | **0** |
| Historical compiler flags | `-O2 -frename-registers -falign-jumps=8` |
| Compiler snapshots | **Jan25 AND Dec06 independently verified** |
| New classification | **EXACT** |

The original 2004 ELF ends this small classifier with the following
case-11 block at relative `+0x42`:

```asm
mov 0x8(%ecx),%eax
cmpl $0x3,0x4(%eax)
jne 0x0805a57e
jmp 0x0805a5a0
```

Earlier original-length C++03 source generated the *semantically
equivalent*, but byte-different:

```asm
cmpl $0x3,0x4(%eax)
je 0x0805a5a0
jmp 0x0805a57e
```

That accounted for the previous **three different bytes** at
relative offsets `0x49`, `0x4a`, `0x4c`.

The new C++03 source expresses the true control-flow topology, without
any register bindings, inline assembly, synthetic data, helper
padding, discarded bytes, or patched instruction encodings:

```cpp
if (__builtin_expect(op->kind == 11, 0)) {
    if (op->operand->kind != 3) goto next_case11;
    return true;
}
next_case11:
if (__builtin_expect(op->kind == 13, 0)) {
    if (op->operand->kind == 3) return true;
}
if (op->kind == 12) {
    if (__builtin_expect(op->operand->kind == 3, 0)) return true;
}
return false;
```

The negative conditional with a normal C++ label reproduces the
original `jne` false edge and unconditional true edge. The original
earlier byte-exact prefix, including both historical assembler
alignment NOPs, is preserved. The source is a period-plausible
reconstruction, not proof that the original author used `goto` or
`__builtin_expect`.

## Bounded negative controls and reproducibility

The new reproducible tests are:

- `vuc/probes/evaluate_accumulator_tail_orientation_oct15d.py`:
  **528** real C++03 inner comparison/source lifetime/prediction
  and compiler-flag combinations; original-sized best 74/77.
- `vuc/probes/evaluate_accumulator_goto_control_oct15d.py`:
  explicitly tests shared true-return label ownership, real
  negative condition fallthrough and genuine branch targets.
  Its **eighth** attempted C++03 source variant, `0-neq-original`,
  produced the new exact **77/77**.

One can reproduce both compilers and the original relocation-aware
strict scorer without re-running the broad search:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -falign-jumps=8 \
    -c "$R/src/IRAccumulatorableExact.cpp" \
    -o "/tmp/vuc-binutils215/accumulator-newexact-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/accumulator-newexact-${snap}.o" \
    --config "$R/config/ir_accumulatorable_probe.json"
done
```

Both snapshots independently output
`length=77`, `exact_bytes=77`,
`different_offsets=[]`, `relocations=0`, `exact=true`.

`vuc/config/ir_accumulatorable_probe.json` now references the
authenticated source under `vuc/src/` and the existing compiler
flags. The candidate path remains selected for diagnostics, and is
newly included in `vuc/config/exact_candidates.json`.
`vuc/tests/test_match_candidates.py` asserts the original address,
size, flags, accepted source location and genuine negative branch,
while the standard original ELF matcher is the authority for complete
machine-code and relocation identity.

## Git state and acceptance

Rebased on latest `origin/main` before editing (the original branch
had already incorporated 140 commits from other VCL reconstruction
work). Preserved **155 unrelated untracked files**, including all
source archives and previous-session experiments. Did not alter
accepted exact functions, VCL root-level code, binary archives,
translation unit attachments, or any pre-existing workspace data.

The original 298-function inventory is now:

- **201 EXACT**, **15,007 original instruction bytes**.
- **24 EXPERIMENTAL**.
- **73 UNRESOLVED**.

Data and BSS acceptance inventories are unchanged: 244 initialized
objects / 71,536 bytes, 12 BSS declarations / 18,437 allocation
bytes. Accepted-source exact matching is a function-level guarantee;
whole-ELF byte-exact linkage and complete binary identity remain
unproven.

## Next practical leads

- `__tcf_1` is 45/46, with a dead pop-register destination difference.
  Source ownership with real preceding methods is more promising than
  arbitrary register forcing.
- `Value::Set(ASTConstValExpr const*)` remains selected 67/455; a
  non-selectable genuine C++03 diagnostic reaches 213 overlapping
  original byte offsets and all 15 reference sites but is 459 bytes.
  Only genuine original-length matches may be promoted.

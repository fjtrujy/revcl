# VUC 2004.1 — Recover 203/219 original Value addition bytes

**Date:** 2026-10-15. **Branch:** `vuc`. Existing Mac repository:
`/path/to/revcl`. Reference: unmodified archived
VUC 2004.1 Linux ELF32 i386 and its original symbol/relocation sites.
Compiler: recovered historical GCC 3.3.3 pre-releases, GNU as 2.15.

## Git and preservation

Started with `git status -uall`, active `vuc` branch at
`ce460a1`, and **163 unrelated pre-existing untracked paths**.
Ran `git fetch origin && git rebase origin/main` successfully
before any tracked changes; no new main commits at that point.
No additional worktrees, subagents, resets, cleanup, original ZIP edits
or root VCL source edits. Do not modify accepted source implementations.

## New authenticated original-length Value::operator+ partial

The previously selected original-size candidate
`vuc/probes/value_addition_partial_oct14.cpp` was
**172/219** original-offset identical bytes, with the authentic
`Value::TypeConvert` relocation.

| Check | Prior candidate | New candidate |
| --- | ---: | ---: |
| Original symbol/address | `_ZNK5ValueplERKS_` / `0x0806e9b0` | Same |
| Original function size | 219 | **219** |
| Compiled candidate size | 219 | **219** |
| Identical original instruction bytes | 172/219 | **203/219 (+31)** |
| Exact contiguous entry prefix | 127 bytes | **169 bytes** |
| Original relocation sites | 1/1 | **1/1** |
| Authenticated destination | `Value::TypeConvert` | `0x0806f7d0` |
| Strict byte-exact status | EXPERIMENTAL | **EXPERIMENTAL** |

The new selected source is
`vuc/probes/ValueAdditionLaneOrderOct15.cpp`; the accepted exact
`vuc/src/ValueArithmeticUnit.cpp` is **not edited**. The new source
retains the real 32-byte local `Value` construction, `TypeConvert`
call, case-1 integer addition, case-3 scalar float addition, and
case-4 four independent float component additions.

The key change is a genuine source-level reorder of **four independent
destination vector lanes**, now emitted in lexical order **3, 1, 2, 0**.
Since `result` and `temporary` are separately constructed local
`Value` instances, this order updates exactly the same component
values. It is a compiler/code-generation hypothesis, not
authentication that the original author wrote that statement order.

Under `-O2 -frename-registers -falign-jumps=1`, the historical
GCC 3.3.3 compiler emits the original **219-byte** function length,
while placing the scalar branch and vector x87 instructions much closer
to the original. This recovers the first **169 complete contiguous
bytes**. The only differing byte offsets now are:

```
169, 172, 175,
194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206
```

No original machine code was pasted or patched. All 16 residual
differences are real compiler emission differences, not acceptably
identical instruction bytes. Other flagged alignments and wrong-sized
candidates are **never treated as strict partials**, even when their
numeric prefix overlaps suggest a much higher score.

Both independently recovered historical compiler snapshots
(January 25, 2004 and December 6, 2003), each with GNU as 2.15,
compiled the final probe source separately with identical result:
`length=219, exact_bytes=203, relocations=1, exact=false`.

The exact manifest is unchanged. The selected experimental manifest
`vuc/config/value_addition_partial_oct14_probe.json` now points
to the new C++ source and its flags. The coverage ledger records the
same original address, original size and EXPERIMENTAL status.
The older partial source remains in `vuc/probes/` for reproducible
negative controls.

## Reproducible cross-target experiment drivers

The new scripts generate **genuine C++03 source variants**, compile
them with the historical toolchain, compare the *original address*
and complete original size and original relocation sites, then
retain only the best strict original-sized object for each run.

| Script under `vuc/probes/` | Builds | Strongest acceptable original-length match |
| --- | ---: | --- |
| `evaluate_type_force_guard_lifetimes_oct15.py` | 1,225 | `Value::TypeConvertForce` **193/257** (unchanged); operand source lifetime/early return/dispatch controls |
| `evaluate_hashcopy_two_arg_ctor_oct15.py` | 324 | Hash copy constructor **80/166** (unchanged); explicit 1–4 argument Node constructor, source next pointer, member writes |
| `evaluate_hashcopy_deferred_tail_oct15.py` | 720 | Hash copy constructor **80/166** (unchanged); real conditional bucket-empty guard, destination-tail pointer lifetime and iteration forms |
| `evaluate_value_plus_branch_owner_oct15.py` | 360 | Addition **172/219** (unchanged); switch/if source control flow and branch-probability hints |
| `evaluate_value_plus_x87_expression_oct15.py` | 875 | Addition **197/219**; recovered an original-length candidate with vector lanes 3,2,1,0 |
| `evaluate_value_plus_all_vector_orders_oct15.py` | 144 | Addition **203/219**, authentic lanes 3,1,2,0 |
| `evaluate_value_plus_203_scalar_controls_oct15.py` | 1,152 | Original-length scalar/int operand temporary and per-lane commutative addition controls; see its guest result JSON |

**4,800** source/compiler recipes if all scripts complete. Source and
relocated object evidence is in independent guest-only scratch
directories under `/tmp/vuc-binutils215/`, with the seven
corresponding scripts writing `results.json`.

The closest baseline Value+ output when using uniform jump
alignment 1 emitted **216** bytes and overlapped up to
**205/216 original-offset bytes**, but this result is *invalid* as
an original-sized match: the original is **219** bytes and the
three missing bytes shift branch addresses and instruction boundaries.
The authentic independent-lane lexical source changes the emitted
function back to 219 bytes, raising the valid score by **31**.
Do not invent NOPs, dummy source stubs or fabricated relocations to
fill the size difference.

For the hash copy constructor, the original stores the 513-bucket
index on the stack and uses a 16-byte Node. The earlier
four-argument hypothetical Node constructor often generates a
larger than original stack frame. Reducing its argument count and
delaying destination-tail formation produces plausible C++03
and original-sized code but no better than **80/166**, even when
a candidate reproduces the original stack-frame size.
The existing hash copy partial and manifest are preserved.
For the 257-byte TypeConvertForce body, genuine local-copy
and boolean guard rewrites do not fix the original FPU-control-word
or caller parameter register allocation differences.
Its selected 193/257 candidate remains untouched.

## Independent regression

```sh
python3 -m unittest discover -s vuc/tests -q

R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -falign-jumps=1 \
    -c "$R/probes/ValueAdditionLaneOrderOct15.cpp" \
    -o "/tmp/vuc-binutils215/valueplus-selected-203-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/valueplus-selected-203-${snap}.o" \
    --config "$R/config/value_addition_partial_oct14_probe.json"
done
```

The authoritative inventory remains
**200 EXACT / 24 EXPERIMENTAL / 74 UNRESOLVED** of 298
named original functions. Strict exact acceptance remains
**200/200** functions and **14,930/14,930** code bytes,
**244/244** initialized objects / **71,536** original bytes,
and **12/12** BSS declarations / **18,437** source allocation
bytes. Whole linked ELF identity remains unproven.

## Next precise lead

The last 16 mismatching Value+ bytes are concentrated at source
offsets +169,+172,+175 and +194–+206, after a 169-byte
authentic prefix. Compare compiler RTL scheduling and
scalar-case label placement against original assembly at
`0x0806ea59` and beyond. Further natural source-level lane
ordering or true translation-unit ownership evidence is justified;
arbitrary alignments, register clamps or padding are not.

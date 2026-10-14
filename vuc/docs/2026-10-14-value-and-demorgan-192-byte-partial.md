# VUC 2004.1: logical AND De Morgan source-order partial, 192/325

**Date:** 2026-10-14. **Branch:** `vuc`. Repository:
`/path/to/revcl`. Original authority: immutable VUC
2004.1 Linux ELF32 i386 in tracked upstream ZIP, analyzed through the
unchanged relocation-aware matcher. No manual instruction bytes, injected
NOPs/alignment-only source filler, register-specific asm, dummy functions,
fabricated symbol addresses or original binary edits were used.

## Repository state and concurrency

At batch start the active branch was `vuc` at `fa88087` with 168
untracked paths. Ran `git fetch origin && git rebase origin/main`:
the branch was up to date. During independent compiler probes, another
work session **separately** staged, validated, committed and pushed
`be393e2 Recover original-length VUC hash copy constructor partial`.
Those changes were preserved and **not restaged or claimed as work in
this batch**. The branch now contains that 80/166 hash-table
constructor partial; the established 163 unrelated untracked paths
remain untouched. Our own probe scripts were kept separate.

No existing accepted exact source, root VCL directory, or original
archive was edited.

## Improved partial: Value::operator&&(Value const&) const

| Property | Before | After |
| --- | --- | --- |
| Address in original executable | `0x0806f340` | `0x0806f340` |
| Original/emitted function size | 325/325 | **325/325** |
| Identical instruction bytes at original offsets | 190/325 | **192/325** |
| Exact contiguous entry prefix | 115 bytes | **115 bytes** |
| Genuine relocation sites | 1/1 | **1/1** |
| Selected manifest | `value_logical_and_partial_oct14_probe.json` | Unchanged |
| Source | `vuc/probes/value_logical_and_partial_oct14.cpp` | Updated |
| Status | EXPERIMENTAL | **EXPERIMENTAL** |

Original `_ZNK5ValueaaERKS_` has a single external call relocation
to `_ZN5Value11TypeConvertERKS_S1_RS_S2_`, target
`0x0806f7d0`. The historical body initializes two real 20-byte
Value objects, checks integer/scalar float types and reduces four
2-element float lanes using genuine x87 boolean comparisons.

The improvement was recovered through a **source-level Boolean
equivalence** rather than altering compiled machine code. For finite
float values, the original form

```cpp
(a0 != 0.0f && b0 != 0.0f) &&
(a1 != 0.0f && b1 != 0.0f) &&
(a2 != 0.0f && b2 != 0.0f) &&
(a3 != 0.0f && b3 != 0.0f)
```

is equivalent to the C++03 De Morgan form

```cpp
!(a1 == 0.0f || a0 == 0.0f || a2 == 0.0f ||
  a3 == 0.0f || b0 == 0.0f || b1 == 0.0f ||
  b2 == 0.0f || b3 == 0.0f)
```

where `a` is the result's float lane and `b` is its temporary
converted counterpart. The real source uses corresponding
`result.values[i]` and `temporary.values[i]` fields. The historical
original compilation is `-ffast-math`, so no original NaN
equivalence is inferred beyond that compiler's assumptions.

This source expression happens to steer GCC 3.3.3's x87 branch
placement into **192** original-offset equal bytes. It does **not**
prove that the 2004 original author wrote the De Morgan form or
the unusual lane order; it is a reproducible optimization hypothesis.
The remaining mismatch begins at original offset **+115**, and the
candidate is **not** promoted into `vuc/src/` or
`vuc/config/exact_candidates.json`.

Historical candidate flags are unchanged:

```sh
-O2 -frename-registers -ffast-math \
-falign-jumps=16 -falign-labels=1 -falign-loops=1
```

The **two recovered independent GCC 3.3.3 snapshots**,
2004-01-25 and 2003-12-06, with GNU as 2.15, each freshly compiled
the final tracked source. The original matcher returned
`length=325, exact_bytes=192, exact=false, relocations=1` on
**both** runs. This is a strict original-length improvement
from 190 to 192 (not the larger, wrong-sized overlapping score
obtained with excessive label alignment).

## Experiment matrix and negative evidence

All experiments operate on actual C++03 constructs with true symbol
and relocation comparisons, via six committed drivers in `vuc/probes/`:

| Reproducible script | Builds | Highest original-length result |
| --- | ---: | --- |
| `evaluate_value_and_source_dispatch_next_oct14.py` | 360 | **190/325**; genuine if/else, independent condition, switch lexical/operand source shapes |
| `evaluate_mltemp_set_lifetimes_next_oct14.py` | 168 | **111/117**, same as prior `MLTempString::Set`; all eight original relocations authenticated, six register differences persist |
| `evaluate_value_and_demorgan_semantics_next_oct14.py` | 260 | **191/325**; found improvement with negated equality OR source algebra |
| `evaluate_hash_copy_outer_index_next_oct14.py` | 216 | **80/166**, same as separately committed provisional hash Node constructor; original four EH/copy relocations authenticate |
| `evaluate_value_and_demorgan_order_next_oct14.py` | 216 | **192/325**; grouping and ordering left/right lane comparisons |
| `evaluate_value_and_independent_lane_order_next_oct14.py` | 576 | **192/325**; all 24 × 24 independent four-lane orders; 432 original-size bodies |

**Total: 1,796 authentic compiler/source recipes.** Scratch C++ and
object files and full `results.json` output are inside the
original `revcl-oracle` Lima guest under the corresponding
`/tmp/vuc-binutils215/` paths. They are not original binary evidence
and are not staged.

The source-dispatch probe found no better original-length `&&`
match. Other alignment schemes sometimes overlap 273 or 291
original-offset bytes, but generate **357–372-byte** bodies instead
of the **325-byte** original, so they fail mandatory length validation.

The separate hash constructor's real bucket-index lifetime remains
unresolved: volatile index forms force a stack value but generate
170-/202-byte bodies, while plain index source variants match at most
80/166. Its selected source and current manifest remain untouched.
The `MLTempString::Set` output register choices are not fixed by
ordinary source aliases, pointer lifetimes or compiler controls.

## Reproduction and regression gates

Reproduce final source independently:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -ffast-math \
    -falign-jumps=16 -falign-labels=1 -falign-loops=1 \
    -c "$R/probes/value_logical_and_partial_oct14.cpp" \
    -o "/tmp/vuc-binutils215/value-and-192-verified-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/value-and-192-verified-${snap}.o" \
    --config "$R/config/value_logical_and_partial_oct14_probe.json"
done
```

The accepted inventory on the updated shared branch is
**200 EXACT / 24 EXPERIMENTAL / 74 UNRESOLVED** of 298 original
named functions, **14,930** fully verified exact instruction
bytes, 244 initialized data objects (71,536 bytes), and 12 BSS
declarations (18,437 source allocation bytes). The extra
experimental hash constructor entered through the separately
authored `be393e2` commit. No linked whole-ELF identity claim.

A useful next direction is the **genuine GCC 3.3 source owner /
peephole control** for the exact opcode and address of the shared
logical AND epilogue at +0xb0. Arbitrary branch probability and
global alignment sweeps are largely exhausted.

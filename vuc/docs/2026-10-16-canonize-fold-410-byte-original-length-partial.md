# VUC Canonize_FoldExpr: first eligible 410-byte experimental reconstruction

Date: 2026-10-16. Original VUC 2004.1 Linux ELF32 at
`0x080502c0`; original symbol
`_Z17Canonize_FoldExprPN2IR9Expr_DataE`.
This function was previously **UNRESOLVED**, not EXPERIMENTAL.
Only the unchanged archived binary is used as the oracle.

## Recovered source structure

The original ELF checks the wrapper kind 6, then its child kind
**5 first, 7 second**. The previously saved
`CanonizeFoldExprNewOct15p.cpp` asked about 7 before 5; that source
compiled to 381 bytes under `-O2 -frename-registers` and had no
eligible original-size candidate. The new
`evaluate_canonize_fold_branch_order_oct16.py` expresses the
original condition precedence through an ordinary nested C++03
`if`. This produces **402** historical compiler bytes in both
plain and rename-register modes, an increase of 21 over the previous
rename-register body, but **still not an eligible original-length
partial**.

The true original emits kind-field stores at object offset +4
before derived vtable installs at +0 when allocating both
`IR::ConstLit_Data` (12 bytes) and `IR::ConstExpr_Data` (16 bytes).
The subsequent
`evaluate_canonize_fold_ctor_branch_oct16.py` combines the
original branch precedence with genuine C++03 base/derived
constructor ownership. An original-style redundant derived
`kind` assignment, on top of a base-class `kind` constructor,
produces **418 bytes** under `-O2` or `-O2 -frename-registers`,
and the original **410-byte length** with historical
`-O2 -frename-registers -falign-jumps=8`. The extra source
assignment is a reconstruction hypothesis, **not evidence that the
original author duplicated the assignment**. This source form is
kept in `vuc/probes/` and remains explicitly experimental.

## Strict binary comparison

Accepted experimental candidate:
`vuc/probes/CanonizeFoldExprBranchKindPartialOct16.cpp`
with manifest
`vuc/config/canonize_fold_expr_partial_oct16.json`.

Both recovered GCC 3.3.3 compiler prereleases (January 25, 2004,
and December 6, 2003) plus GNU as 2.15 independently reproduce:

| Requirement | Jan25 | Dec06 |
| --- | ---: | ---: |
| Original length | 410/410 | 410/410 |
| Identical original-offset code bytes | **72/410** | **72/410** |
| Authenticated original relocation sites | **19/19** | **19/19** |
| Source literal bytes mapped to original `.rodata` | **31/31** | **31/31** |
| Byte-exact function | No | No |

The original `"illegal operand in const expr!"\0` diagnostic
is verified at `0x08072460`. The symbol destinations include
the original `error` and `std::cout` globals, both original
polymorphic IR vtables, `operator new`, `operator delete`,
`MLGameString` copying, both recursive calls,
`IsFoldable`, `ConcatenateBinOp`, and `_Unwind_Resume`.
No arbitrary literal padding or binary edits are used.

The **72 bytes** are modest; this is not close to exactness.
The first mismatches include stack frame allocation and different
local register/temporary lifetimes. The original stack reserves
`0x48` bytes after the frame pointer while this candidate
reserves `0x38`. The original first constant-child copy saves
a source string pointer before allocation, whereas the candidate
generally calculates it afterward. This suggests a *real*
source-level lifetime/argument evaluation difference for a later
round. Do not spend further effort merely permuting conditions
already shown compiler-equivalent.

## Reproduce inside revcl-oracle

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -falign-jumps=8 \
    -c "$R/probes/CanonizeFoldExprBranchKindPartialOct16.cpp" \
    -o "/tmp/vuc-fold-original-length-${snap}-oct16.o"
  python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-fold-original-length-${snap}-oct16.o" \
    --config "$R/config/canonize_fold_expr_partial_oct16.json"
done
```

This is a newly selected original-size **EXPERIMENTAL** candidate.
It remains outside `vuc/src/` and `exact_candidates.json`,
which still accepts **204** fully exact functions and **15,855**
instruction bytes. The 298-symbol queue becomes
**204 EXACT / 39 EXPERIMENTAL / 55 UNRESOLVED**.
No whole-linked-ELF byte identity is established.

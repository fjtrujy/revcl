# Recovering the VUC Value.cpp startup wrapper from genuine arithmetic neighbors

Date: 2026-10-13; branch `vuc`.

## Original startup wrapper: 31/31 exact

The original `_GLOBAL__I__ZN5ValueC2Ev` at `0x0806f9d0`, size **31**,
previously matched **29/31** instruction bytes in an isolated `Value.cpp`
source. Its remaining differences were the GCC 3.3.3-selected scratch
register used to pass `0xffff` to the local initializer.

A more complete ordinary C++ source-unit fragment now restores the
original register allocation. It contains the original constructor
(`Value::Value()`), `Value::Set(ASTExpr const*)`, and reconstructed
`Value::operator+`, `Value::operator*` and signed relational/comparison
members. These are **actual named VUC methods**, not dummy functions or
invented machine instructions. The wrapper matches **31/31 bytes** with
one relocated call to the original
`__static_initialization_and_destruction_0(int,int)` at `0x0806e6f0`.
The historical compiler wrapper is GCC 3.3.3 (2004-01-25) with GNU
as 2.15. The successful flags are:

```
-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections
```

The source fragment is recorded as `vuc/src/Value.cpp` and its accepted
manifest as `vuc/config/value_global_genuine_context_oct13.json`.

## Compiler-source controls

`vuc/probes/evaluate_value_full_unit_oct13.py` tests 13 meaningful
original C++ method groups under four GCC 3.3.3 options. The first
full match occurs with the original constructor, addition,
multiplication and relational/comparison definitions in this order.
`vuc/probes/evaluate_value_set_context_oct13.py` adds the already exact
ASTExpr dispatcher in five different positions. All five variants
preserve the 31-byte global wrapper and both 44-byte original
constructor clones under the wrapper's flags. An ordinary compilation
of the same source with `-O2 -falign-jumps=1` also reproduces
`Value::Set(ASTExpr const*)` **42/42**; without explicit jump alignment
it instead emits 54 bytes. These are tested source/flag hypotheses,
not authentication of the original full-file build command.

The partial arithmetic/multiplication implementations remain
**experimental** and are not newly counted as exact. The exact
comparison functions retained in `vuc/src/ValueComparison.cpp` are
independently accepted under their original-specific flags. The
fragment's duplicated definitions must eventually be reconciled when
building one complete linked executable; acceptance here is per
original function record, not a claim of complete translation-unit
fidelity.

## Evidence and next steps

The original has 298 named function records. This milestone brings
the exact manifest to **172/172**, covering **11,677/11,677** instruction
bytes; the ledger reports 172 EXACT, 7 EXPERIMENTAL, 119 UNRESOLVED.
The original “Value.cpp” object includes many additional methods,
notably `Set(ASTConstValExpr const*)`, `ToString`, and type-conversion
members, whose source is not reconstructed byte-exact. Recovering
those structures and consolidating the final translation-unit boundary
remain important for final ELF byte identity.

Reproduce with the installed VM toolchain:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-172-value-oct13
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_value_full_unit_oct13.py
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_value_set_context_oct13.py
```

The untouched canonical original archive remains the only authority for
a future whole-ELF byte-exact acceptance; matching one compiler-generated
wrapper is **not** linked-executable equality.

# VUC Canonize_TryReduceExpr: new signed/unsigned 116/359 partials

Date: 2026-10-15. Target: original VUC 2004.1 Linux i386.
These two template functions were previously UNRESOLVED and had no
accepted or selected experimental C++ sources:

- `Canonize_TryReduceExpr<unsigned int>` at `0x0804ffb0`,
  **359 bytes**;
- `Canonize_TryReduceExpr<int>` at `0x08050120`,
  **359 bytes**.

## Recovered original behavior

The ELF dispatches on `IR::Expr_Data::kind` at object offset +4.
The visible kinds are:

- 5: binary expression. Initialize two integer temporaries, recursively
  try both operands, and call the appropriate newly recovered
  `ReduceBinOp<T>` template on the operator at offset +8.
  Store a successful-reduction flag and return it.
- 6: unary/name wrapper. For non-7 inner expressions, recursively reduce
  the child at offset +12. For inner kind 7, copy the MLGameString
  at child offset +8. Repeatedly resolve the name using the real
  `localEnv` then `globalEnv` tables, which are original ELF
  named BSS objects at `0x0808b460` and `0x0808ac40`.
  If the first character is a digit, parse decimal with
  `__strtol_internal` and return true; otherwise return false.
- Other kinds: return false.

The original function calls `MLGameString`'s copy constructor,
`MLHashTable<MLGameString,MLGameString>::FindPtr`, `c_str`,
`isdigit`, `__strtol_internal`, itself recursively and
`ReduceBinOp<T>`. All 13 original relocation sites were
authenticated by symbol name and original ELF addresses.

The first baseline declaration placed the kind-6 arm before kind-5
and was far from the original control flow. Reordering source arms
according to the disassembly and recovering an ordinary local success
indicator brought both targets close to the original **359-byte
length**.

## Strict candidates and limits

The source `vuc/probes/CanonizeTryReduceNewOct15p.cpp` now uses a
C++03 `switch` for the real two active kinds, retaining all original
external ownership. Compiler options: historical
`-O2 -frename-registers -falign-jumps=4 -falign-labels=2
-falign-loops=1`. Both independently recovered GCC 3.3.3
snapshots with GNU as 2.15 produce:

| Target | Size | Original-offset bytes | Relocations |
| --- | ---: | ---: | ---: |
| unsigned | 359/359 | **116/359** | 13/13 |
| signed | 359/359 | **116/359** | 13/13 |

These are only limited partial matches. Matching original length and
relocations alone is **not** a byte-exact function. The remaining
instruction mismatch includes stack-local lifetime scheduling and
branch ordering, especially the early return and the named lookup
loops. Do not promote to `vuc/src` or `exact_candidates.json`.
The status becomes **204 EXACT / 32 EXPERIMENTAL /
62 UNRESOLVED** over 298 functions.

The earlier if/if C++ source was 111/359 when correctly sized; the
authentic two-case switch reaches 116/359. An exploratory source/flag
ledger `vuc/probes/evaluate_canonize_reduce_fresh_oct15p.py`
tests regular C++03 success-local declarations (bool, int, T),
normal recursive-branch sequencing and different stack lifetime
patterns. It found no exact candidate; the results and generated
ELF32 objects remain in the existing Lima guest under
`/tmp/vuc-binutils215/canonize-new-source-idioms-oct15p/` and
`/tmp/vuc-binutils215/canonize-branch-layout-oct15p/`.
The original `ReduceBinOp<T>` and actual global/local hash-table
source-unit context are more promising next steps than further
compiler-flag permutations.

Reproduction (existing Lima guest):

```sh
R=/path/to/revcl/vuc
/tmp/vuc-binutils215/g++-jan25-as215 \
  -O2 -frename-registers -falign-jumps=4 \
  -falign-labels=2 -falign-loops=1 \
  -c "$R/probes/CanonizeTryReduceNewOct15p.cpp" \
  -o /tmp/vuc-binutils215/CanonizeTryReduceNewOct15p.o
for suffix in unsigned signed; do
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/CanonizeTryReduceNewOct15p.o \
    --config "$R/config/canonize_try_reduce_${suffix}_partial_oct15p.json"
done
```

The original complete linked VUC ELF still has no established
byte-identical reconstruction.

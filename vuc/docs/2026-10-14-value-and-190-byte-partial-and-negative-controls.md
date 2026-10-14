# VUC 2004.1: Value logical AND 190/325 partial and bounded cross-target controls

**Date:** 2026-10-14. **Branch:** `vuc`, existing Mac repository
`/path/to/revcl`. The original is the unmodified
archived VUC 2004.1 Linux x86 ELF32, verified by the VUC decompilation
and strict relocation-aware byte matcher. No assembly stubs, hand-coded
NOPs, register-constraint tricks, fabricated relocations, or original
ELF patches were used.

## Repository and main

Checked active branch, recent commits and `git status -uall` before
modifying files. The starting `vuc` HEAD was `d9ce461`,
with **163 unrelated pre-existing untracked files**, which remain
untouched. Executed `git fetch origin && git rebase origin/main`
successfully: current `origin/main` had no newer commits. Neither
root VCL sources nor previously exact VUC sources or original archives
were edited.

## Improvement: `Value::operator&&(Value const&) const`

| Property | Prior selection | Improved selection |
| --- | ---: | ---: |
| Original x86 ELF address | `0x0806f340` | `0x0806f340` |
| Original function length | 325 | **325** |
| Emitted candidate length | 325 | **325** |
| Original-offset matching instruction bytes | 188/325 | **190/325** |
| Contiguous original exact entry prefix | 115 | **115** |
| Authenticated relocation sites | 1/1 | **1/1** |
| Byte-exact acceptance | No | **No: EXPERIMENTAL** |

The selected ordinary C++03 implementation remains
`vuc/probes/value_logical_and_partial_oct14.cpp`, with the same
legitimate `Value` type layout, `TypeConvert` call, integer boolean
conjunction, scalar float conjunction, and four-lane interleaved
float-pair conjunction. The selected manifest remains
`vuc/config/value_logical_and_partial_oct14_probe.json`, and the exact
manifest remains unchanged. The single authenticated
`_ZN5Value11TypeConvertERKS_S1_RS_S2_` relocation resolves to
**`0x0806f7d0`**.

The improvement is a real historical GCC 3.3.3 branch alignment,
not a source-level byte insertion. The selected compile control changes
from jump alignment **1**, label alignment **8** to jump alignment
**16**, label alignment **1**; loop alignment remains **1**:

```sh
-O2 -frename-registers -ffast-math \
-falign-jumps=16 -falign-labels=1 -falign-loops=1
```

Compiling the *tracked selected source*, independently of matrix
objects, using both GCC 3.3.3 prerelease snapshots (2004-01-25 and
2003-12-06) plus GNU as 2.15 returned identically:
`length=325, exact_bytes=190, exact=false, relocations=1`.

Original disassembly positions the common epilogue at function offset
**+0xb0**; the natural selected object still places it at **+0xa8**.
This unresolved eight-byte topology/alignment difference affects
branch displacements and subsequent x87 float checks. A 16-aligned
label variant emits **357 bytes** and overlaps **273 original-offset
bytes**, but *cannot* be accepted or counted as a stricter partial:
it is 32 bytes too long. It would be misleading to treat its high
overlap as a 273/325 success. The selected candidate is 190/325, not exact.

## Seven reproducible bounded investigation drivers

All the following are retained in `vuc/probes/`, with generated
C++/ELF objects and detailed JSON output strictly in Lima guest scratch
`/tmp/vuc-binutils215/`.

| Script | Builds | Result |
| --- | ---: | --- |
| `evaluate_value_and_large_align_oct14.py` | 80 | Original-length best **190/325**; controlled jump, label, loop and block reorder alignment |
| `evaluate_value_and_prediction_oct14.py` | 108 | Compiler-generated branch probability and switch hints do not improve 188/325; no evidence historical authors used such hints |
| `evaluate_value_and_case_order_oct14.py` | 108 | All six genuine `switch` lexical orders: best original-length **190/325** |
| `evaluate_value_and_boolean_groups_oct14.py` | 64 | Two GCC snapshots and C++03 pair/group boolean lifetimes: still **190/325** |
| `evaluate_value_division_extended_align_oct14.py` | 64 | Authentic `Value` arithmetic source-unit context; original-length best **145/235** in this narrower flag grid, **worse** than pre-existing selected 178/235; no change |
| `evaluate_hash_copy_large_alignment_oct14.py` | 64 | Hash-table copy constructor, all 4 original relocations: bodies 135–162 bytes, **none** the original 166 bytes; remains unresolved |
| `evaluate_value_and_compiler_passes_oct14.py` | 27 | Compiler optimization and actual i386-family CPU choices: original-length best remains **190/325** |

**Total 515** compiler/source configurations. Most were compiled under
GCC 3.3.3 January 2004; the true boolean grouping controls and
hash-table alignment checks also include independent December 2003
GCC builds. Unsupported original-size outputs and compilation
failures are recorded as negative controls, not promoted.

The division test is purposefully a **negative** result: the selected
`Value::operator/` original-sized **178/235** partial in
`vuc/probes/ValueDivisionUnit.cpp` and its manifest are preserved.
The hash constructor original 166-byte function remains without an
eligible original-length candidate; a compiler alignment change alone
does not reconstruct its original exception/stack lifetime.

## Acceptance boundaries and reproducibility

The improved `Value::operator&&` stays **EXPERIMENTAL**. Previous
**200 byte-exact functions**, including the completely exact
`Value::operator||` at `0x0806f490`, retain their original objects,
instructions, and configured relocations.

Reproduce the selected partial independently:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -ffast-math \
    -falign-jumps=16 -falign-labels=1 -falign-loops=1 \
    -c "$R/probes/value_logical_and_partial_oct14.cpp" \
    -o "/tmp/vuc-binutils215/value-and-190-verified-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/value-and-190-verified-${snap}.o" \
    --config "$R/config/value_logical_and_partial_oct14_probe.json"
done
```

Run the exact production regression through
`vuc/scripts/match_candidates.py`, followed by the initialized data
and BSS checkers and all host Python tests. The canonical function
ledger remains **200 EXACT / 23 EXPERIMENTAL / 75 UNRESOLVED** out of
298 original named symbols. The exact code count remains
**14,930/14,930**, named data **244/244** (71,536 bytes), and BSS
**12/12** (18,437 source allocation bytes). Whole linked ELF byte
identity is still unproven.

## Next technical lead

Recover original compiler block-frequency / jump-label provenance
rather than adding alignment-only source constructs. For
`Value::operator&&`, the common epilogue's +0xb0 placement is a
useful discriminator. GCC dumps of the accepted `Value::operator||`
compile can help identify which real control-flow edge would produce
the original `+0xa0`/`+0xb0` label layout, but
the unrelated full function must remain independently validated.

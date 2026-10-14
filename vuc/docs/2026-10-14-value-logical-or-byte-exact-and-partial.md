# VUC 2004.1 Value logical operators: exact OR, partial AND

Date: 2026-10-14. Branch `vuc` in the existing Mac repository
`/path/to/revcl`. Original authority is the immutable
2004.1 Linux ELF32 i386 extracted by the checked VUC harness from the
tracked, checksum-verified upstream archive. No original binaries,
hand-crafted opcodes, fake external symbol addresses, or dummy compiler
functions were used.

## Upstream and historical GCC

The initial working tree was `vuc`, with exactly 163 pre-existing
untracked paths; no tracked modifications. Ran
`git fetch origin && git rebase origin/main` successfully after new
VCL main commit `01d48ed` (`oDecode.cpp` reconstruction).
Read and considered the new
`docs/matching-notes.md` and VCL implementation lesson, but did not
transfer its GCC 3.2.2 assumptions directly to VUC's original
GCC 3.3.3-era code. The original untracked
`vuc/config/value_logical_aa_probe.json` was preserved; the selected
AND candidate uses a **distinct tracked manifest**.

The authoritative compilers are both separately recovered 2004-era
GCC 3.3.3 pre-releases: **2004-01-25** and **2003-12-06**, each paired
with the recovered GNU assembler **2.15**. This matches historical
code-generation signatures but the original Debian-patched GCC
configuration and author's precise source flag set remain unknown.

## Exact: Value::operator||(const Value&) const

| Property | Independently verified |
| --- | --- |
| Original symbol | `_ZNK5ValueooERKS_` |
| Original executable address | `0x0806f490` |
| Original function length | **309** instruction bytes |
| Fresh Jan 25 GCC result | **309/309**, full exact |
| Fresh Dec 6 GCC result | **309/309**, full exact |
| External relocations | **1/1**, `Value::TypeConvert` at `0x0806f7d0` |
| Source | `vuc/src/ValueLogicalOr.cpp` |
| Manifest | `vuc/config/value_logical_or_exact_oct14_probe.json` |
| Status | **EXACT, admitted to exact and selected manifests** |

Reconstructed source uses the same original 20-byte `Value` header
layout and concrete constructors accepted for existing arithmetic,
bitwise and relational operations. It creates a default `Value`
result/temporary, sets result's boolean type tag (2), invokes the
existing original `TypeConvert` member, and dispatches actual
integer, scalar float and 4-lane float cases.

**Important source semantics recovered from original x87 control
flow:** the vector logical OR does *not* OR all eight component
truth values. Its original branch sequence tests each two-input
component with OR, and then combines the four component-pair
truth results with AND:

```cpp
result.bits =
    (result.values[0] != 0.0f || temporary.values[0] != 0.0f) &&
    (result.values[1] != 0.0f || temporary.values[1] != 0.0f) &&
    (result.values[2] != 0.0f || temporary.values[2] != 0.0f) &&
    (result.values[3] != 0.0f || temporary.values[3] != 0.0f);
```

Initial authentic C++ reconstruction incorrectly OR'ed all four
pair-results, producing **279/309 original bytes with a 196-byte
contiguous original prefix** under original-sized alignment controls.
The original ELF's repeated x87 `fcom/fnstsw/sahf` and branch
fallthrough at `0x0806f545–0x0806f5c3` proved the actual
pair-OR / group-AND behavior. Correcting the real C++ boolean
expression, **not** adding padding or patched bytes, recovered the
entire 309-byte function and the authentic relocation.

Selected historical flags, reproduced under both recovered snapshots:

```sh
-O2 -frename-registers -ffast-math \
-falign-jumps=8 -falign-labels=1 -falign-loops=4
```

These options are measured successful reconstruction controls, not a
claim to have recovered the original Makefile spelling. The exact
candidate is a standalone genuine C++03 compilation, not a guessed
whole-ELF identity.

## New partial: Value::operator&&(const Value&) const

| Property | Independently verified |
| --- | --- |
| Original symbol | `_ZNK5ValueaaERKS_` |
| Original executable address | `0x0806f340` |
| Original function length | **325** |
| Candidate size | **325**, original length |
| Original-offset identical bytes | **188/325** |
| Original exact entry prefix | **115 bytes** |
| External relocations | **1/1**, same `TypeConvert` |
| Source | `vuc/probes/value_logical_and_partial_oct14.cpp` |
| Manifest | `vuc/config/value_logical_and_partial_oct14_probe.json` |
| Status | **EXPERIMENTAL ONLY** |

The natural C++03 implementation checks boolean integer and scalar
floats, and 4-lane float conjunction, again using the recovered
`Value` ABI and genuine `TypeConvert`. The selected original-length
reconstruction uses
`-O2 -frename-registers -ffast-math -falign-jumps=1
-falign-labels=8 -falign-loops=1`.
Both period GCC snapshots independently emit the same 188/325 result.
A more naturally aligned candidate shares up to 291 overlapping
original-offset bytes, but emits 341 bytes (not 325) and therefore
**is not accepted or promoted**. The AND source stays in
`vuc/probes/` and is absent from the exact manifest.

## Fully reproducible investigation

- `vuc/probes/evaluate_value_logical_reconstruct_oct14.py`:
  **180** authentic original-era C++03 compiles: 90 attempts per
  operator, varying integer boolean spelling, float boolean form,
  vector expression owner and historical optimization flag controls.
  Full JSON and generated C++/ELF objects stay in
  `/tmp/vuc-binutils215/value-logical-reconstruct-next-oct14`.
- `vuc/probes/evaluate_value_logical_alignment_oct14.py`:
  **128** controlled historical GCC compiles, 64 per operator,
  varying jump, label, loop alignment and legitimate block reorder
  without inline asm or fabricated NOPs.
  Evidence in `/tmp/vuc-binutils215/value-logical-alignment-next-oct14`.
- The two final source files were **independently compiled again**
  under the January and December 2003/2004 compiler snapshots,
  without using the experiment-generated .o files. The matching
  original data relocation `_ZN5Value11TypeConvertERKS_S1_RS_S2_`
  was checked at its ELF-verified original address in all four
  independent compiles.

To reproduce the exact result:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -ffast-math \
    -falign-jumps=8 -falign-labels=1 -falign-loops=4 \
    -c "$R/src/ValueLogicalOr.cpp" \
    -o "/tmp/vuc-binutils215/value-logical-or-verified-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/value-logical-or-verified-${snap}.o" \
    --config "$R/config/value_logical_or_exact_oct14_probe.json"
done
```

Reproduce AND with the `value_logical_and_partial_oct14_probe.json`
flags (same snapshots). Full VUC gates are
`vuc/scripts/match_candidates.py` on its exact manifest, then
`match_data_candidates.py`, `match_bss_candidates.py` and
`python3 -m unittest discover -s vuc/tests -q`.

## Coverage and next step

The correctly updated canonical ledger is **200 EXACT,
23 EXPERIMENTAL, 75 UNRESOLVED** out of **298** original named
functions. The exact code gate now covers **14,930/14,930 original
instruction bytes**, up from 14,621. Exact named initialized data
continues to cover **244/244 objects (71,536 bytes)**; **12/12**
named BSS declarations (18,437 source allocation bytes). A
standalone byte-exact function is not proof of whole-linked-ELF
identity.

Next focus: compare original `Value::operator&&` float and vector
basic-block ordering against its valid **325-byte, 188-byte**
partial, using the now-proven `Value::operator||` scalar and
vector branch-control precedents. Do not relax byte-exact admission
criteria or copy its flags blindly.

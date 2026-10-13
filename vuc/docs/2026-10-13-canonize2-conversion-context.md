# VUC Canonize2_Reduce: original 85-byte function via genuine conversion predecessors

Date: 2026-10-13; branch `vuc`.

## New exact application function

`Canonize2_Reduce(IR::Module_Data*)`, original `0x0804ff50`, is now
**85/85 byte-exact**, with all **three** ELF32 relocations resolved:
`globalEnv` (`0x0808ac40`), the original `MLHashTable::clear()`
(`0x08050ca0`), and `Canonize_Reduce(IR::Fragment_Data*)`
(`0x08050460`). It is accepted in
`vuc/config/canonize2_context_exact_oct13.json` from ordinary C++ in
`vuc/src/ConvVal.cpp`.

## Source-context evidence and negative controls

The original VUC `Canonize2.cpp` has named `ConvVal` overloads just
before the canonization functions (original addresses `0x0804f5d0`,
`0x0804f610`, `0x0804f650`). The isolated canonicalization helper
uses the same algorithm and 85-byte length, but the January 2004
GCC 3.3.3 compiler chooses **EDX** rather than original **EAX** for
the initial module-status load and test: **83/85** matching bytes.

Compiling the unchanged, already-reconstructed ordinary C++ numeric
conversion functions before `Canonize2_Reduce` restores the original
register allocation and produces **85/85 exact bytes** without changing
the helper's own algorithm, patching bytes, adding dummy predecessor
functions, or introducing compiler-register constraints. Its three
conversion neighbors also remain byte-exact in the *same* historical
compiler object:

| Function | Original address | Result |
| --- | --- | --- |
| `ConvVal(MLGameString const&, float&)` | `0x0804f5d0` | 51/51 |
| `ConvVal(MLGameString const&, int&)` | `0x0804f610` | 60/60 |
| `ConvVal(MLGameString const&, unsigned&)` | `0x0804f650` | 60/60 |
| `Canonize2_Reduce(IR::Module_Data*)` | `0x0804ff50` | **85/85 newly accepted** |

The reproducible `vuc/probes/evaluate_canonize_context_oct13.py`
compares six *genuine*-source compositions against the same original
85-byte target:

| Source context | Exact instruction bytes |
| --- | ---: |
| Isolated canonizer | 83/85 |
| IR predicates, then canonizer | 83/85 |
| ConvVal overloads, then canonizer | **85/85** |
| IR predicates, ConvVal, canonizer | **85/85** |
| ConvVal, IR predicates, canonizer | **85/85** |
| Canonizer before the genuine neighbors | 83/85 |

This establishes a compilation-order/code-generation dependency: the
numeric converter definitions, not unrelated predicates, are enough.
The original GCC 3.3.3 source compiler and GNU assembler 2.15 were
used with `-O2 -frename-registers -falign-jumps=1`, not VCL's
GCC 3.2.2 toolchain. Omitting the jump-alignment flag generated
**101 bytes** for the canonizer in this combined source, so the
accepted per-function flags remain explicitly documented hypotheses.

The standalone conversion source fragment was extended directly, rather
than creating duplicate definition owners. The historical translation
unit's full source (including long expression canonization methods and
its static initializer) is still incomplete. This source fragment
must eventually be unified with the remaining `Canonize2.cpp`
functions and global environment owner for complete ELF linking.

## Acceptance and limitations

The exact manifest has **171** original function-symbol records and
**11,646/11,646** verified instruction bytes. The 298-record ledger
is **171 EXACT**, **7 EXPERIMENTAL**, **120 UNRESOLVED**. Existing
conversion exact-match manifests now use the same compilation-unit
flags as the accepted canonicalization helper; the strict gate
revalidates all accepted functions. The source fragment is a partial
reconstruction of the original translation unit, not whole-ELF
byte identity.

Reproduce with the compiler already installed in `revcl-oracle`:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-canonize-171
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_canonize_context_oct13.py
```

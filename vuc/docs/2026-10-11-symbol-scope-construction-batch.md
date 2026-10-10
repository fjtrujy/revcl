# VUC exact symbol-scope construction and adjacent source investigations

Date: 2026-10-11. Branch: `vuc`.

## Accepted function

The canonical ZIP-backed VUC 2004.1 ELF function
`SymbolTable::BeginScope()` at **0x0806a470** is now reproduced
**140/140 instruction bytes**, with all **five relocations** resolved
against the real original symbol targets. The original code constructs
a temporary 513-bucket `MLHashTable<MLGameString,SymbolTableEntry*,...>`
on the stack and appends it to the `std::vector` scope stack.

The key change is recognizing the **out-of-line copy constructor**
`MLHashTable<...>::MLHashTable(const MLHashTable&)`. The original
calls it at `0x0804f420`; our provisional class previously allowed
GCC to inline a bulk copy and emitted a 191-byte body instead of
the original 140. Declaring the copy constructor in the shared
`vuc/src/MLHashTable.h` (without supplying a fabricated body) and
using ordinary C++ `scopes_.push_back(scope)` reproduces the
original code, including its exception-unwind path and the original
`_M_insert_aux` call. The implementation lives in
`vuc/src/SymbolTableBeginScope.cpp`.

The original constructor's complete implementation has not been
authenticated or reconstructed. The provisional class type and source
translation-unit boundaries are not claims about the original
headers.

## Whole accepted-function gate

Using GCC 3.3.3 20040125 (prerelease), GNU as 2.15 and the original
ELF symbols:

- **142/142 independently checked original function records exact**
- **7,611/7,611 instruction bytes identical**
- **326 independently resolved ELF32 relocations**
- **78 verified literal bytes**
- **56 passing host tests**
- Canonical inventory: **142 EXACT, 11 EXPERIMENTAL, 145 UNRESOLVED**
  of 298 named original text-function records.

No whole-binary percentage is asserted. The independent strict
`whole_elf_gate.py` still has **no reconstructed complete ELF** to
accept. Byte-for-byte equality of the entire ELF remains mandatory.

## Clean-checkout reproducibility fix

The interrupted earlier milestone had three config paths and an accepted
`AsmVariable` source fragment absent from the Git index. A local VM could
compile those files, but a clean clone could not. These original batch
inputs, the new accepted `BeginScope` source/config and the previously
missing `currFileName` exploratory wrapper have been explicitly added
to Git. `test_manifest_sources_and_configs_are_committed` verifies that
**both** exact and exploratory manifest configurations, and every referenced
C++ source, are versioned whenever the unit suite is run in a Git checkout.
It does not mark the non-exact wrapper as accepted.

## Explicitly unaccepted neighboring candidates

The function `MLGameStringTable::Get()` has a plausible ordinary
C++ local-static singleton implementation and original named BSS
relocations. The best version still reproduces only **102/106**
instruction bytes. A reproducible sweep in
`vuc/probes/evaluate_get_types.py` tested **147** combinations of
32-bit field types, volatility and constructor ordering and found no
complete match. These failures are code-generation/register-allocation
differences and remain outside the exact-only manifest.

The neighboring GCC 3.3
`std::__uninitialized_copy_aux<MLHashTable*,MLHashTable*>()`
instantiation now compiles with the original non-inlined copy
constructor and correct template symbol name. Its two tested jump
alignments produced 127 and 115 bytes, versus the original 120.
The exploratory source and manifest are retained in
`vuc/probes/hash_scope_uninitialized.cpp` and
`vuc/config/hash_scope_uninitialized_probe.json`.

The deep-copy constructor itself is modeled in
`vuc/probes/hash_copy_constructor.cpp`; the current result is
162 bytes instead of the original 166, so it is **not accepted**.

## Reproduce

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-142
```

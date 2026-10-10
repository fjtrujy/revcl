# VUC singleton hash-table destructor recovery

Date: 2026-10-11. Branch: `vuc`.

## Accepted byte-exact destructor callback

The original ELF local function `__tcf_0` at `0x08071e00` is a
compiler-generated **22-byte** callback which destroys the function-local
static `MLGameStringTable::Get()::gst` by calling the named
`MLHashTable<const char*,MLGameStringData*,...,513>::clear()`
at `0x08071ea0`.

A normal, period-plausible C++ `static MLGameStringTable gst` singleton
inside the getter, with a destructor inherited from the hash-table base,
makes GCC 3.3.3 (20040125 prerelease) and GNU as 2.15 emit exactly
**22/22 identical instruction bytes** for the local destructor callback.
The ELF32 relocations resolve independently to:

- original named singleton `_ZZN17MLGameStringTable3GetEvE3gst`
  at `0x0808bd00`;
- original named bucket cleanup function at `0x08071ea0`.

The fragment is kept in `vuc/src/MLGameStringTableLifecycle.cpp`
and the accepted manifest is
`vuc/config/ml_game_string_table_cleanup_probe.json`.

**Important:** This file also compiles a `MLGameStringTable::Get()`
candidate that **does not** match its original 106-byte body. That
candidate is *not accepted* or included in the strict manifest.
The independent exploratory config
`vuc/config/ml_string_table_get_probe.json` remains explicitly
unaccepted. A regression test checks this separation. The source is
a provisional reconstruction of the ABI and lifetime mechanism, not
an authenticated complete original translation unit.

No assembly bytes, register-forcing instructions or artificial
padding are embedded in the C++ source.

## Strict checkpoint

After the preceding accepted `SymbolTable::BeginScope()` addition,
the exact-only gate now validates **143/143 original function-symbol
records** and **7,633/7,633 instruction bytes**, including **328
resolved ELF32 relocations** and **78 verified original literal bytes**.
The 298-symbol ledger is **143 EXACT, 11 EXPERIMENTAL, 144 UNRESOLVED**.
These figures count only named original functions, not the whole ELF.

The project's independent `whole_elf_gate.py` remains the mandatory
final acceptance gate. Complete linked executable byte-for-byte identity
has not been achieved.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-143
```

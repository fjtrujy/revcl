# VUC milestone: byte-exact string data and hash-table primitives

## Verified results

The byte-exact function suite increased from 15/15 (438 bytes) to
**20/20 accepted functions (828/828 original instruction bytes)** using
the source-date GCC 3.3.3 20040125 compiler in the existing Linux VM.
These are only the explicitly reconstructed functions, **not** a
whole-binary match.

Five newly accepted function-symbol entries:

| Original function | Address | Exact bytes | Verified relocations |
| --- | --- | ---: | ---: |
| `MLGameStringData::MLGameStringData(char const*, bool)` C2 | `0x080715e0` | 111/111 | 4 |
| `MLGameStringData::MLGameStringData(char const*, bool)` C1 | `0x08071650` | 111/111 | 4 |
| `MLHashFun<char const*>::IsEqual()` | `0x08071f40` | 40/40 | 1 |
| `MLHashTable<int,int,...,513>::clear()` | `0x080715a0` | 64/64 | 1 |
| `MLHashTable<char const*,MLGameStringData*,...,513>::clear()` | `0x08071ea0` | 64/64 | 1 |

The constructors exactly reproduce the observed object layout and
allocation behavior: 32-bit hash at offset 0, string pointer at offset 4,
string length at offset 8, and temporary ownership flag at offset 12.
For non-temporary strings they allocate `length + 1` characters and use
`strcpy`; temporary strings reuse the supplied pointer. The source
uses ordinary C++ and the original calls to `strlen`,
`MLHashFun<char const*>::Hash`, `new[]`, and `strcpy`. Their four
relocations each are resolved against original ELF addresses.

The `IsEqual` specialization calls `strcmp`. The two `clear()`
specializations use 513 buckets and a linked-list node layout with
`next` at offset 12; each resolves its `operator delete` relocation.
The comparison is exact with `-O2 -frename-registers -falign-jumps=1`.
The jump-alignment option is a **candidate compiler flag hypothesis**,
not proof of the original full translation-unit flags.

This adds 390 verified code bytes and 11 resolved relocations. The full
accepted set now has **22 resolved relocations** and **19 independently
checked literal bytes**, in addition to its 828 exact code bytes.

## Reproduce

From the host, using the existing Lima VM and compiler bootstrap:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-gcc333-jan25/g++-jan25-candidate \
  --out /tmp/vuc-gcc333-jan25/exact-gate-20

python3 vuc/scripts/coverage.py --write
python3 -m unittest discover -s vuc/tests -v
```

The exact gate must return zero, with all twenty functions exact.
Any changed byte or an unresolved relocation is a failure.
The broader exploratory manifest still includes the non-exact
`AsmOutput_WritePrologue` and must return nonzero.

The comprehensive symbol inventory is **20 EXACT, 1 EXPERIMENTAL,
277 UNRESOLVED** out of 298 ELF text function symbols. Those counts
are *not* an overall reconstruction byte percentage.

## Remaining work

`MLHashFun<char const*>::Hash()` (86 bytes) was investigated with
an evidence-driven 3-bit rotate loop. The current hypothesis emits
87 bytes with different register choices and is explicitly
**not** accepted; it remains in `vuc/probes/hash_cstring.cpp`.

`MLGameStringTable::Get()` and `Add()`, the string hash function,
string-temporary helpers, the remaining original data and linker
layout are unresolved. A complete historical Debian compiler and
linker build is not yet authenticated.

Do **not** treat these matches as byte-for-byte identity of the original
`MLGameString.cpp` translation unit, much less the complete VUC ELF.

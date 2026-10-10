# Byte-exact VUC currFileName static initialization and whole-ELF gate

Date: 2026-10-10. Branch: `vuc`.

## Newly accepted original helper

The original ELF contains a separate `currFileName` initialization
translation unit. Its GNU GCC 3.3.3 helper
`__static_initialization_and_destruction_0(int,int)` at
`0x0806f9f0` is now reproduced exactly:

| Property | Result |
| --- | --- |
| Original code bytes | **88/88 exact** |
| ELF32 relocations | **6 resolved** |
| Original `std::ios_base::__ioinit` object | `0x0808bcbc` |
| Original `error` byte | `0x0808bccc` |
| Original `__tcf_0` callback | `0x08071570` |
| Original `currFileName` pointer | `0x08077b80` |
| Original pointer target | `0x08072286` (empty string) |

The ordinary C++ hypothesis in
`vuc/src/CurrFileInitialization.cpp` defines the original globally
visible file-name pointer, original error byte and a trivial static
initializer that clears the error flag. The historical compiler
generates the iostream initialization, `__cxa_atexit` registration,
and the zero-byte assignment in the exact original order and with
every instruction byte correct.

**Important qualification:** the compiler-generated
`_GLOBAL__I_currFileName` wrapper at `0x08071580` is *not* accepted.
Its current source is **29/31 exact bytes**, with the original using
`%edx` where our toolchain uses `%eax`. The wrapper is separately
listed in the experimental manifest and original-symbol ledger.
The independently exact `__tcf_0` iostream callback remains
**16/16** in the combined candidate object.

Source-form experiments covering char pointers, const pointers,
void pointers, volatile pointers, char arrays, different original-size
error-byte types and multiple optimizer flags did **not** solve
the wrapper's register assignment. Combining the candidate with the
already accepted `script_lex()` function preserved the exact
88-byte helper and 39-byte lexer wrapper, but still did not fix the
31-byte global initializer. No register forcing, source-embedded
machine code, invented padding or incorrect acceptance is used.

The original entire translation unit and header set remain
provisional, as do all emitted data layout details not independently
checked by the function matcher.

## Independent whole-executable acceptance gate

`vuc/scripts/whole_elf_gate.py` adds an independent, byte-for-byte
acceptance contract for a **finished linked VUC ELF**:

```sh
python3 vuc/scripts/whole_elf_gate.py \
  --candidate /absolute/path/to/rebuilt-vuc
```

The tool reads the original executable **directly from the tracked
upstream ZIP**, then verifies its known canonical SHA-256 and ELF32
i386 identity through `decomp.load()`. It compares the supplied
candidate's *entire file byte sequence* to that authenticated original,
with no section filtering, relocation substitution or alignment
masking. This includes ELF headers, program/section tables, symbol
and string tables, executable code, all initialized data, relocation
and linker information when present, section gaps and padding.

It prints a machine-readable report with:

- `acceptance: PASS` or `FAIL`
- `whole_elf_byte_exact` boolean
- Both complete-file SHA-256 hashes and sizes
- The first unequal file offset, or null for an exact match

Exit status is **0 only for full byte equality**, **1 for any
mismatch**, and **2 when verification could not run** (missing
candidate, corrupt original, etc.). In particular, byte-perfect
matching of all currently accepted functions **does not make this
gate pass**; it requires a finished reconstructed executable.

The existing unit tests independently verify that a byte-for-byte
copy of the canonical original passes the *comparator's own
self-test*, while changing a single code byte, a single
read-only-data byte, the last file byte or the candidate size
causes failure. These self-tests exercise the comparator, **not**
a rebuilt compiler output, and must never be described as evidence
that whole-ELF reconstruction is complete.

The authentic reference has **314,338 bytes** and SHA-256
`14c77997057f279ab25a3591c6a379d6e4744f2b1551271d43602948c037d8cf`.

## Regression and remaining scope

With this one new accepted original helper, the historical
source-date GCC strict function gate reaches **136/136 named
function records**, **7,073/7,073 instruction bytes**, **309
independently verified relocations**, and **36 original literal
bytes**. The host unit suite now contains **56 passing tests**.

The 298-entry original symbol inventory records **136 EXACT,
11 EXPERIMENTAL and 151 UNRESOLVED**. These are symbol records,
not a percentage of the executable. There is still no complete
rebuilt VUC executable for which the new independent whole-ELF
gate could be applied meaningfully. Reconstructing that executable,
the original compiler/linker inputs, and *every missing source
function and original data section* remains mandatory.

Reproduce the accepted function gate:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-136
```

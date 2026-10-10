# VUC batch: original x86 alignment and string helpers

## Exact-match acceptance

The recovered **GCC 3.3.3 20040125 (prerelease)** frontend, combined with
GNU **as 2.15**, now reconstructs **25/25 accepted VUC function symbols**
byte-for-byte (**1,084/1,084 function instruction bytes**).

This batch added five independently verified functions:

| Function | Original address | Exact bytes | Relocations |
| --- | --- | ---: | ---: |
| `MLHashFun<char const*>::Hash()` | `0x08071ee0` | 86/86 | 1 |
| `MLHashFun<MLGameString>::IsEqual()` | `0x080719b0` | 24/24 | 0 |
| `MLTempStringData::FormatString()` | `0x08071a20` | 35/35 | 1 |
| `MLTempString::operator+=()` | `0x08071d80` | 53/53 | 2 |
| `MLTempString::operator+()` | `0x08071dc0` | 58/58 | 2 |

The complete accepted set verifies **28 relocations** and **29
literal bytes checked per manifest** (the `%s%s` literal is inspected
twice, once for each operator; this is not 29 distinct file bytes).
These are not percentages of the entire executable.

The `operator+=` and `operator+` functions reference the original
`%s%s` literal at `0x080727df`, together with calls to `Set()`
and the formatted-string constructor respectively. The
`FormatString()` forwarding function uses the original varargs ABI,
and the hash-string equality helper uses an inlined equality operator;
using direct pointer comparison alone emits a shorter, **non-exact**
function. The hash algorithm combines three successive 1-bit right
rotations with each signed character value.

## Why binutils 2.15 matters

The previous GNU assembler 2.40 in the Debian 12 Lima VM generated
different instruction-alignment padding for the hash loop, even though
the compiler's core instructions were correct. After reconstructing
the source-level variable initialization order and bit type, GCC 3.3.3
with modern GNU as 2.40 produced 86 bytes with only **77/86** bytes
equal to the original. The differing offsets were alignment/padding
NOP encodings around the loop: modern GNU as emits `66 90` and
`lea` sequences in a different order.

Debian's archived `binutils_2.15-6_i386.deb` contains GNU as 2.15.
Its internal alignment expansion produces **86/86 exact bytes**
without writing any assembler manually or fabricating padding in the
C++ source. The *same* old-assembler wrapper has now passed the
complete 25-function exact regression set.

Reproducible download:

- URL: https://archive.debian.org/debian/pool/main/b/binutils/binutils_2.15-6_i386.deb
- SHA-256: `f4a73c5ad1cba4c61b38bd83e6b0e493cc6a1a41d30bc010ae4969617d12bc9b`
- Extraction and wrapper script: `vuc/scripts/bootstrap_binutils215.sh`

The Debian package contains a 32-bit `as` executable linked with
`libbfd-2.15.so`; the reproducible wrapper sets its isolated
`LD_LIBRARY_PATH`. It never installs or overwrites the guest system
assembler.

**This does not authenticate GNU as 2.15 as VUC's actual original
assembler.** Debian's documented 2004 compiler environment also used
earlier binutils releases. Matching one instruction-padding pattern,
even across multiple exact function bodies, is insufficient to
authenticate the entire 2004 toolchain, original compiler flags, object
link order, or any part of the final ELF layout.

## Source-fragment ordering matters

For `MLTempString::operator+()`, the same C++ expression compiled in
isolation can match the original, but inserting it before the existing
methods in the combined provisional source fragment changed register
assignments, yielding only 45/58 equal bytes. Defining it **after the
other accepted methods** restored a byte-for-byte match without any
hand-written machine instructions.

This is important evidence for future full translation-unit recovery,
but no reconstructed source fragment is yet asserted to be a complete
original compilation unit.

## Reproduce in the existing Lima VM

```sh
limactl shell revcl-oracle -- sh \
  /path/to/revcl/vuc/scripts/bootstrap_binutils215.sh \
  /tmp/vuc-binutils215 /tmp/vuc-gcc333-jan25 \
  /tmp/vuc-gcc335/root/usr/include/c++/3.3

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-25

python3 vuc/scripts/coverage.py --write
python3 -m unittest discover -s vuc/tests -v
```

The canonical VUC archive is the SHA-256-verified ZIP-backed original;
all compiled objects and the assembler are kept in scratch storage.
Never modify the ZIP or replace its original ELF.

## Function queue and next work

The canonical ELF symbol table still contains **298 function-symbol
records**. The exact-only manifest now accepts 25; the unmatched
`AsmOutput_WritePrologue` remains experimental, and the other **272**
symbols are unresolved. The complete ELF still has **no validated
binary-wide matching percentage** or exact rebuilt counterpart.

The next valuable work is reconstructing `MLGameStringTable::Get()`
and `Add()`, plus the remaining formatted-string implementation and
original data. Exact linking, runtime libraries, and historical
compiler/assembler provenance remain mandatory for ultimate
whole-executable byte equality.

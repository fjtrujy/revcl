# VUC milestone: byte-exact MLGameString member-function cluster

## What has been completed

All **nine** named `MLGameString` member-function entries in the verified
ELF symbol table are now reconstructed from C++ and pass the
byte-exact GCC 3.3.3 function comparison gate.

| Member function | Original address | Exact code bytes | Relocations |
| --- | --- | ---: | ---: |
| Default constructor C2 | `0x080716c0` | 14/14 | 0 |
| Default constructor C1 | `0x080716d0` | 14/14 | 0 |
| `(char const*, bool)` constructor C2 | `0x080716e0` | 52/52 | 2 |
| `(char const*, bool)` constructor C1 | `0x08071720` | 52/52 | 2 |
| Copy constructor C2 | `0x08071760` | 15/15 | 0 |
| Copy constructor C1 | `0x08071770` | 15/15 | 0 |
| `(MLTempString const&)` constructor C2 | `0x08071780` | 49/49 | 2 |
| `(MLTempString const&)` constructor C1 | `0x080717c0` | 49/49 | 2 |
| `c_str() const` | `0x08071800` | 13/13 | 0 |

**9/9 exact entries, 273/273 code bytes, eight resolved calls.**

For `MLTempString const&`, the source must capture the pointer **before**
calling `MLGameStringTable::Get()`. Keeping the expression entirely inside
the initializer results in a shorter, *non-exact* body. This is a
source-semantic reconstruction backed by observed function bytes, not a
manual insertion of arbitrary machine code.

## Scope and acceptance

The complete accepted candidate set across all provisional VUC C++ files is
now **15/15 byte exact** (438/438 code bytes, 11 resolved relocations and
19 checked literal bytes). The exact-only runner exits nonzero whenever an
accepted function differs even by one byte. The separate exploratory list
contains the still-unmatched `AsmOutput_WritePrologue`, and correctly
**fails** acceptance.

These results are *function-body* matches. The `MLGameString.cpp` **source
file/translation unit as a whole is not complete**: `MLGameStringData`,
`MLGameStringTable`, `MLTempString`, and related helpers within that
neighborhood still contain unreconstructed or unverified functions, globals,
and data. Further, the exact `-O2 -frename-registers` flag hypothesis is not
proof of VUC's original per-translation-unit flags.

## Exhaustive follow-up tracking

The original ZIP-backed ELF has 298 named function-symbol entries. Run
`python3 vuc/scripts/coverage.py --write` to regenerate
`vuc/config/function_status.tsv`, the **complete work queue**:

- EXACT: 15
- EXPERIMENTAL: 1 (prologue, nonexact)
- UNRESOLVED: 282

These are symbol-status counts, *not* code-byte percentages or an assessment
of whole-executable fidelity. The work queue includes startup and runtime
symbols and some zero-size symbols; those are not silently omitted or
claimed as reconstructed. The separately recovered `STT_FILE` inventory
(`vuc/config/source_units.tsv`) contains 22 VUC `.cpp` filenames and
79 explicitly local text functions. Do **not** infer definite global-symbol
ownership from symbol-table grouping without additional evidence.

## Next milestone

Continue from `MLGameStringData::MLGameStringData(char const*, bool)` and
`MLGameStringTable::Get()/Add()`, then the string-temporary functions.
Promote **only** fully byte-identical functions to `vuc/src/` and the exact
manifest, preserving this milestone. Independently recover exact period
linker, binutils, libc and libstdc++ data before claiming byte-for-byte
identity of the complete ELF file.

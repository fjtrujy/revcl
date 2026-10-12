# Exact inputFileName / ListFile printing source-unit recovery

Date: 2026-10-12. Branch: `vuc`.

## Original ELF evidence

The canonical ZIP-backed VUC 2004.1 executable includes the following
separately identified symbols and bytes:

| Symbol | Original address | Exact bytes |
| --- | --- | ---: |
| `inputFileName` (256-byte initialized buffer) | `0x08077860` | 256/256 |
| `ListFile::PrintLine(int,std::ofstream&)` | `0x0805aad0` | 133/133 |
| `_GLOBAL__I_inputFileName` | `0x0805b700` | 31/31 |

All three now come from **one ordinary C++ compilation unit**:
`vuc/src/ListFilePrintLine.cpp`. The source includes `<iostream>`,
defines the original 256-byte `inputFileName` buffer, and implements
the already recovered `ListFile::PrintLine` body.

The historical GCC 3.3.3 compiler snapshot with GNU as 2.15 emits all
three fully exact with a *single* candidate flag set:

```
-O2 -frename-registers -fuse-cxa-atexit \
-ffunction-sections -fno-zero-initialized-in-bss
```

This is stronger than the earlier independent results:

- A translation unit with only the `inputFileName` definition emitted
  a **29/31** initialization wrapper, choosing `%eax` rather than
  the original `%edx` for the `0xffff` argument.
- Adding the recovered `ListFile::PrintLine` implementation to the
  translation unit made the wrapper **31/31** exact while retaining
  the previous **133/133** function match and **256/256** data match.
- A second experiment with the recovered `ListFile::Print()` methods
  produced the same exact wrapper, consistent with compilation-unit
  context affecting GCC's old register-renaming heuristic.

The wrapper's ELF32 PC-relative call relocation resolves to the
authenticated original `__static_initialization_and_destruction_0ii`
helper at `0x0805b390`. The named data object is checked independently
under the strict data matcher with its original address and section.
No linked addresses or instructions are hardcoded into candidate source.
The linker and original helper body have not been reconstructed here.

To avoid two owners for the same global, `inputFileName` is no longer
declared in `GlobalBuffers.cpp`; its exact-data manifest is redirected
to this unit. The other buffers remain defined and checked in
`GlobalBuffers.cpp`.

## Scope and limitations

The combined source unit reproduces the three **named original objects**.
It is not yet proof of the complete historical `ListFile.cpp` source
or exact ordering of unrelated functions and symbols in the original
object. The end goal remains exact code, data, linkage and ELF metadata
for the **complete executable**, not just function and data manifests.

## Reproduction

```sh
python3 -m unittest vuc.tests.test_match_candidates vuc.tests.test_coverage -q
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/gate-inputfilename-166
```

The `inputFileName` data object is also independently verified through
`vuc/scripts/match_data_candidates.py`.

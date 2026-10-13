# VUC ListFile global startup: exact wrapper from source-level Generate reconstruction

Date: 2026-10-13. Branch: `vuc`.

## New function-level exact match

The recovered ordinary C++ file in `vuc/src/ListFileUnit.cpp` now
produces `_GLOBAL__I__ZN8ListFile8GenerateEPKc` at original
`0x0805b020` as **31/31 matching instruction bytes**, including its
single ELF32 relocation to the original `__static_initialization_and_destruction_0(int,int)`
at `0x0805a5c0`. The accepted source uses the January 25, 2004 GCC
3.3.3 compiler wrapper and GNU assembler 2.15 with
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`.

The fragment defines the genuine `ListFile` methods `Generate`,
`PrintLine`, `Print`, and `GetNumLines` in the corresponding original
method order. `Generate` uses a conventional `std::ifstream`, an
in-memory 256-byte line buffer, `getline` and a vector of strings.
These choices are supported by original ELF calls to the historical
GCC 3.3 file buffer, 256-byte getline, and string-append operations.
However, the original `Generate` is **1,206 bytes**, while the
candidate body is **989 bytes**; it is **not** accepted as exact and
is not claimed to implement the entire original algorithm.

The following methods are checked independently in the same combined
candidate compilation unit:

| Original function | Check |
| --- | ---: |
| `ListFile::GetNumLines()` | 18/18, no relocations |
| `ListFile::PrintLine()` | 133/133, 7 relocations, 3 literal bytes |
| Compiler-generated `ListFile::Generate` global initializer | **31/31 new** |

The `ListFile::Print()` method also matches **88/88**, with two
relocations, if compiled from this source with the separate
`-falign-jumps=1` setting. Under this latter flag the 133-byte
`PrintLine` instead emits 131 bytes. This demonstrates that the
complete historical per-unit flags and exact compilation provenance
are still unresolved; independent exact-function manifests remain
unchanged for the other methods.

The candidate only reached the expected global constructor name once a
real `ListFile::Generate` definition appeared as the first source-unit
method. Without it, GCC generated a constructor keyed to `PrintLine`.
The earlier source containing `PrintLine`, `Print`, and `GetNumLines`
did not satisfy the original symbol name and cannot serve as a
whole-source match. `vuc/probes/listfile_real_unit_oct13.cpp` retains
that development hypothesis without fabrication or binary editing.

## Remaining constraints

This increases strict coverage to **175/298 original functions** and
**11,770/11,770 instruction bytes**, with six experimental and
117 unresolved function records. No source was inserted solely to
influence instruction bytes: all recovered methods correspond to
original retained VUC symbols. The complete historical `ListFile`
algorithm, source headers, linker, and whole-ELF byte identity remain
open reconstruction tasks.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-listfile-175
```

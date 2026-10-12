# Exact historical runtime and streambuf ABI constants

Date: 2026-10-12; branch `vuc`.

The canonical ZIP-backed VUC ELF has four additional small, independently
named `.rodata` objects. They have been reproduced from ordinary C++ and
individually validated against the authentic ELF, without copying binary
data or substituting linked addresses:

| Original ELF symbol | Virtual address | Bytes | Source evidence |
| --- | --- | --- | --- |
| `_fp_hw` | `0x08072080` | 4/4 | historical i386 glibc CRT marker, value 3 |
| `_IO_stdin_used` | `0x08072084` | 4/4 | historical GNU startup ABI marker, value 0x20001 |
| `std::basic_streambuf<char>::_S_pback_size` | `0x0807223c` | 4/4 | GCC 3.3 historical `<streambuf>`, value 1 |
| `std::basic_streambuf<wchar_t>::_S_pback_size` | `0x08072240` | 4/4 | GCC 3.3 historical `<streambuf>`, value 1 |

The historical GCC 3.3 standard C++ header automatically instantiates
both `_S_pback_size` objects from a simple `#include <streambuf>`.
No fabricated hand-built binary object, inline assembly, manually
serialized pointer or artificial padding was used. The two glibc/CRT
ABI values are ordinary four-byte named `extern "C" const int`
declarations; they have no relocations.

Accepted fragment sources:
`vuc/src/RuntimeAbiConstants.cpp` and
`vuc/src/StreambufPbackConstants.cpp`, each with its own strict
data-object manifests.

The accepted data gate now verifies **244/244 original named data
objects and 71,536/71,536 bytes**, with **390 resolved
relocations**. The 166-function exact gate is unchanged, with
11,314/11,314 instruction bytes. All 90 host tests pass.

Reproduce in the existing `revcl-oracle` Lima VM:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-abi-244-oct12
```

These four exact original symbol-object comparisons do **not**
establish original CRT object/source provenance, whole-section
layout, or complete executable equality. The original compiler
patches, full source translation units, linker, parser/lexer
tables and remaining functions must ultimately be matched
byte for byte for the entire VUC executable.

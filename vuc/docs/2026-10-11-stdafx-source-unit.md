# VUC StdAfx.cpp: exact GCC 3.3 source-file-keyed constructor

Date: 2026-10-11. Branch: `vuc`.

## Evidence and original symbols

The canonical ZIP-backed VUC 2004.1 ELF contains an original
`STT_FILE` record for `StdAfx.cpp`. The reproducible
`vuc/config/source_units.tsv` inventory identifies this translation
unit's three original text symbols, in order:

| Original symbol | Address | Exact bytes |
| --- | --- | --- |
| `__static_initialization_and_destruction_0(int,int)` | `0x0806a260` | 66/66 |
| `__tcf_0` (iostream cleanup) | `0x0806a2b0` | 16/16 |
| `_GLOBAL__I_StdAfx.cppxtsrAa` | `0x0806a2c0` | **31/31 (new)** |

A normal C++ source file named `StdAfx.cpp` and containing
`#include <iostream>` reproduces all three code bodies in one
historical GCC 3.3.3 object using:

```
-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections -falign-jumps=1
```

The original BSS `_ZSt8__ioinit` at `0x0808bcac`, the cleanup
call to `std::ios_base::Init::~Init()`, five initializer relocations,
and the constructor's call to `0x0806a260` are all mapped to
authenticated original addresses. The accepted manifest now points
all three records to `vuc/src/StdAfx.cpp`; the ELF-derived
`iostream_cleanup_inventory.py --write` generator reproduces those
source/flag selections deterministically.

The exact original toolchain command line is not authenticated;
the documented flags remain a *working original-era compilation
hypothesis*. A reconstructed original translation unit is not the
same as a completely linked, identical original executable.

## Randomized local function name

GCC 3.3 creates a local constructor symbol using the source-file name
plus a fresh six-character temporary suffix. The original symbol
uses `xtsrAa`; another run of the same historical compiler emitted,
for example, `_GLOBAL__I_...StdAfx.cppMoVYF4`. The suffix is not a
part of the C++ program and cannot be reconstructed by changing
program logic.

The matcher now supports `compiler_generated_unit: "StdAfx.cpp"`
solely for this class of source-file-keyed `_GLOBAL__I_` local
constructor. It:

1. Matches the *canonical original* function name, address and size
   exactly in the authenticated ELF;
2. Requires the original spelling to have the specified source-file
   basename and an alphanumeric six-character suffix;
3. Finds exactly one local `STT_FUNC` candidate with the same
   source-file basename and compiler-generated suffix pattern;
4. Checks the entire candidate body byte-for-byte after resolving
   each original function relocation;
5. Rejects ambiguous candidates, unrelated file names and global
   symbols with forged local-generator names.

This is a narrowly scoped **source provenance comparator**, not an
alias to a fabricated original function or permission to omit
mismatched bytes. The regression tests validate the naming pattern,
canonical `StdAfx.cpp` file-symbol inventory and failure modes.

## Exact-gate status

Following this new function, the original function-symbol inventory
is **156 EXACT, 9 EXPERIMENTAL, 133 UNRESOLVED**, out of 298 original
text function records. The strict suite passes **156/156 named
records, 9,403/9,403 instruction bytes**, **403 resolved
relocations**, and 132 verified per-candidate literal bytes.
This does **not** mean the entire original ELF is byte identical.

Reproduce in the existing VM:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/iostream_cleanup_inventory.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-156-stdafx
```

The canonical entire-ELF gate remains unpassed. The mandatory final
goal is full binary byte equality including every original code
function, global/static data, linker layout, ELF metadata and padding.

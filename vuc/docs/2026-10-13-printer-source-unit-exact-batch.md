# Byte-exact VUC Printer source-unit reconstruction

Date: 2026-10-13; branch `vuc`.

## Accepted new functions

The original `Printer.cpp`-owned C++ function ordering supplies genuine
compiler context for two previously non-exact/unresolved functions:

| Original symbol | Original address | Instruction bytes | Relocations |
| --- | --- | ---: | ---: |
| `Printer_UglyPrint(ASTModule*, std::ostream&)` | `0x0805cb40` | **40/40** | 2 |
| `_GLOBAL__I__Z5PrintP10ASTFunCallRSo` | `0x0805cb80` | **31/31** | 1 |

Both were compiled by the recovered GCC 3.3.3 (2004-01-25) wrapper using
GNU as 2.15, with authentic ordinary C++ source in `vuc/src/Printer.cpp`.
The accepted compilation flags are `-O2 -frename-registers
-fuse-cxa-atexit -ffunction-sections`; the original Debian compiler
patchset and linker remain incompletely authenticated.

## Why the source-unit context matters

The isolated historical C++ implementation of `Printer_UglyPrint()`
was **38/40**: it loaded the original newline literal into ECX rather
than EAX. Neither renaming temporary variables nor the 72 tested
source-expression/flag combinations could repair the two register bytes.

Original ELF symbol order has `Print(ASTFunCall*, std::ostream&)` at
`0x0805b770` before `Printer_UglyPrint()` at `0x0805cb40`. Compiling
both genuine C++ functions in that order inside the same translation
unit changes GCC 3.3.3's register allocation/renaming context: the
latter function becomes **40/40**. No dummy predecessor, filler,
inline assembly, artificial alignment, binary patching, or hardcoded
original machine bytes were used. Unlike a cosmetic byte workaround,
the preceding function is also a real 98-byte original VUC symbol.

The combined source currently produces `Print(ASTFunCall*)` at
**92/98** with its 7 relocations and 9 mapped literal bytes verified.
It is not promoted. Its isolated `-O2` probe previously reached
96/98 but did not reproduce the accepted module printer or exact
source-unit context. This is a useful partial source reconstruction,
not a new exact function.

Including `<iostream>` rather than only `<ostream>` also emits the
historical compiler-generated global iostream initialization wrapper
at original `0x0805cb80`. With both genuine member printers in the
same source unit it becomes **31/31** rather than the prior isolated
29/31 register-choice near-matches observed for other VUC startup
wrappers. Its single relocation resolves to the original function
`__static_initialization_and_destruction_0(int,int)` at `0x0805b720`.
The wrapper and module printer remain byte-exact in the same candidate
object with the same compiler flags.

For original literals, the module printer's newline is mapped to
`0x08073070`; the combined source unit also emits the authenticated
"(", ", ", and ")" AST function-call literals. The strict comparator
checks all nine literal bytes via four section-range mappings; no
invented addresses or permissive wildcard mappings are used. The
original references must still be linked correctly in a future
full-executable build.

## Accepted metadata and testing

The exact manifest now includes
`vuc/config/ugly_printer_unit_oct13_probe.json` and
`vuc/config/iostream_global_printer_oct13_probe.json`, both owned by
`vuc/src/Printer.cpp`. The selected-candidate manifest lists the same
symbols and preserves all remaining experimental functions separately.
The original-function status ledger now reports:

- **168 EXACT** (11,385/11,385 instruction bytes)
- **8 EXPERIMENTAL**
- **122 UNRESOLVED**
- 298 original named function records in total.

Reproduce the complete strict gate:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-printer-168
```

This milestone is two original function-symbol matches, **not**
byte equality for the linked historical 314,338-byte ELF. Runtime,
linker, compiler-generated objects, many application functions and
whole-file section ordering are still incomplete.

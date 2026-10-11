# Byte-exact VUC binder startup recovery — October 11, 2026

This checkpoint finishes interrupted work without resetting any existing files.
The canonical ELF remains the immutable VUC 2004.1 i386 executable in the
tracked upstream ZIP. Only complete, relocation-verified function matches
enter the accepted manifest.

## Accepted additional original functions

| Symbol | Original address | Verified instructions |
| --- | --- | --- |
| `__tcf_1` (scope-vector cleanup) | `0x0804ebc0` | 107/107 |
| `_GLOBAL__I_exportModule` | `0x0804ec30` | 31/31 |
| `__static_initialization_and_destruction_0(int,int)` (export-module unit) | `0x0804a940` | 393/393 |

The three functions are ordinary compiler-generated C++ lifecycle functions
from the provisional binder source fragment
`vuc/src/ScopeVectorLifecycle.cpp`. The scope vector initializes with a
reserve of 100 elements; the unit also initializes
`enclosingFunID`/`enclosingLoopID` strings. It uses original named BSS
object symbols and correctly records the original `std::ios_base::Init`,
`std::vector::reserve`, hash cleanup, `__cxa_atexit`, exception/unwind
and other linked targets. The 393-byte initializer manifest maps its three
original `.rodata` literal ranges independently. The new tests reject
mutations of those literal bytes.

These are **three original function records**, not complete reconstruction
of the binder translation unit or whole executable. Per-function compiler
flags in manifests establish reproducible local matches but do not prove
the historical link command or source-file partitioning.

## Validation

Date-matched GCC 3.3.3 20040125 and GNU as 2.15:

- **146/146** original function records match every instruction byte.
- **8,164/8,164** instruction bytes identical.
- **364** resolved original relocations.
- **132** verified referenced literal bytes.
- Original function-symbol inventory: **146 EXACT, 10 EXPERIMENTAL,
  142 UNRESOLVED**, total 298.

The full ELF has **not** been reconstructed and no whole-binary percentage
or completion claim is made. A passing candidate-function gate is a
necessary but insufficient condition for the mandatory whole-ELF match.

Run:

```sh
python3 -m unittest discover -s vuc/tests -q
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-resume-146
```

The source/configuration manifest commitment test deliberately checks the
current Git HEAD, rather than accepting untracked or merely staged sources;
it passes after this checkpoint is committed, ensuring clean clones contain
all accepted candidate inputs.

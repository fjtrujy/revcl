# Recovering exact VUC C++ RTTI class inheritance from original ELF

Date: 2026-10-12. Branch: `vuc`.

## Independently exact original data records

The verified VUC 2004.1 ZIP-backed ELF exposes **81** named Itanium C++
ABI `_ZTS...` class type-name strings and **81** corresponding
`_ZTI...` type-information objects. All **162** RTTI symbols now match
byte-for-byte in independently compiled ordinary C++ candidate objects,
using the historical GCC 3.3.3 toolchain and strict original-section
relocation resolution.

| RTTI record family | Objects | Exact bytes | Linked relocations |
| --- | ---: | ---: | ---: |
| `_ZTS...` class-name strings | 81 | 1,324/1,324 | 0 |
| `_ZTI...` type-info records | 81 | 936/936 | 234 |
| Previously accepted initialized objects | 9 | 68,364/68,364 | 1 |
| **Total exact named data** | **171** | **70,624/70,624** | **235** |

These figures are **named object-level matches**, not a complete
section, full linked ELF, or whole-binary matching score. Type-info
bytes must not be confused with newly reconstructed function bytes.

## Original single-inheritance graph

Each original type-info record is read directly from the immutable
ELF's `.rodata` section:

- **Nine roots** contain two 32-bit words: the relocated
  `__cxxabiv1::__class_type_info` vtable address-point and the
  matching original `_ZTS...` name-string address.
- **72 single-base derived classes** contain a third 32-bit pointer
  to another authenticated `_ZTI...` symbol. Their first pointer
  refers instead to the original `__si_class_type_info` vtable
  address-point.
- The two vtable symbols are imported from historical
  `GLIBCPP_3.2` and remain **dynamic library dependencies**, not
  vtables whose implementation has been reconstructed in REVCL.
  Source-level references to unversioned names are reconciled with
  the **exact versioned symbols** in the original ELF; arbitrary
  unmatched aliases are rejected.

This graph provides original evidence for relationships such as
`ASTVarDecl : ASTDecl`, `yyFlexLexer : FlexLexer`, and
`IR::Add_Data : IR::Operator_Data`. It does **not** establish
object sizes, member offsets, virtual method order, method bodies,
or class source-file ownership.

The `rtti_name_inventory.py` generator validates the original
symbol identity, name-length encoding, exact object size, section
address, typeinfo ABI pointer, linked name pointer, and original
parent-typeinfo pointer. It rejects missing or invented parent types,
unexpected roots, cross-group parent references, or cyclic
declaration graphs. The emitted source fragments use ordinary
`struct Derived : public Base {}` declarations and
`typeid(...)` references, never embedded address constants, raw
machine code or fabricated ELF bytes.

## Exact data acceptance and test requirements

The manifest `vuc/config/exact_data_candidates.json` now lists
**171 exact** independently identified symbols: 9 initialized
objects and 162 RTTI data symbols. The strict data matcher resolves
all 235 incoming ELF32 relocations against original linked addresses,
including the 8-byte C++ ABI vtable address-point addends. The
source-backed types are split between `ASTRttiNames.cpp` and
`IRRttiNames.cpp`, with the other 9 data objects compiled from the
reconstructed global-buffer, counter and filename fragments.

Host tests verify all original bytes and metadata independently,
check regenerated manifests and sources, and ensure a corrupted
original inheritance pointer is rejected. The type-info generator
performs a topological declaration order to make ordinary
single-inheritance C++ compile deterministically.

## Reproduce

```sh
python3 vuc/scripts/rtti_name_inventory.py --write
python3 -m unittest discover -s vuc/tests -q
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-typeinfo-171-oct12
```

**Mandatory final objective remains byte-exact identity of the
complete linked executable** (code, data, sections, relocation/
dynamic metadata, symbols, padding and headers). A 171/171
named-data acceptance result alone does not meet that objective.

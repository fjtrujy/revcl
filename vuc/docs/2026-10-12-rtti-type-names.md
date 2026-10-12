# VUC original RTTI type-name data: 81 exact C++ ABI objects

Date: 2026-10-12. Branch: `vuc`.

## Original ELF metadata and compiler reconstruction

The authenticated VUC 2004.1 ELF contains **81 named, nonempty
`_ZTS...` C++ RTTI type-name objects** in its file-backed
`.rodata` section:

- **51** names in the `IR::` namespace (e.g. `IR::UInt_Data`)
- **30** AST, lexer and other top-level class names (e.g.
  `ASTNode`, `yyFlexLexer`, `FlexLexer`)

All original objects use the historical GCC 3.3 Itanium C++ ABI
mangling. For each original `_ZTS...` symbol, the actual linked
original data bytes are exactly the ABI-encoded class-name text plus
one NUL byte. Their original `STT_OBJECT` addresses, lengths,
section types and literal contents are authenticated directly from
the canonical ZIP member, not from extracted or user-owned copies.

The deterministic `vuc/scripts/rtti_name_inventory.py` reads the
canonical ELF symbol table, validates and decodes the names, then
writes two ordinary C++ compilation fragments:

- `vuc/src/IRRttiNames.cpp`: name-only declarations for the 51
  `IR::` classes
- `vuc/src/ASTRttiNames.cpp`: name-only declarations for the 30
  top-level classes

Each fragment declares empty classes and uses ordinary `typeid(T)`
global references to force historical GCC to emit genuine
`_ZTS...` objects. **No byte arrays of the original names, invented
machine instructions, inline assembly, or historical virtual addresses
are embedded into source code.** Compiler-generated RTTI names are
matched against all original `STT_OBJECT` names and bytes through
`match_data.py`; 81 source-specific JSON manifests record the actual
linked original symbol addresses.

The two empty class-definition collections are **ABI type-name
probes only**, not claims about original virtual methods, inheritance,
object layout, typeinfo or vtables. The compiler also emits
auxiliary RTTI objects and global typeinfo references, which are
not yet accepted as matching complete original translation units.

## Exact acceptance

The independent initialized-data gate now verifies:

| Data type | Original object records | Verified bytes |
| --- | ---: | ---: |
| Zero-filled text, path and assembly buffers | 6 | 68,352 |
| Initialized `InstructionList` counters | 2 | 8 |
| Relocated `currFileName` pointer to original NUL literal | 1 | 4 |
| Newly reconstructed compiler-generated RTTI type names | **81** | **1,324** |
| **Total accepted original initialized-data objects** | **90/90** | **69,688/69,688** |

Exactly **one** original initialized-data relocation is checked,
belonging to `currFileName`. The RTTI *names* are literal bytes and
have no relocations within their own objects. The matching of 1,324
additional RTTI bytes is not an increase in original matched
machine-instruction bytes.

The original C++ inheritance/typeinfo objects remain a subsequent
separate task. The current strict function gate remains **165/165**
named original function records, **11,283/11,283 instruction bytes**.
That is not complete executable byte equality.

## Reproduction and negative tests

```sh
python3 vuc/scripts/rtti_name_inventory.py --write
python3 -m unittest discover -s vuc/tests -q

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-rtti-90-oct12
```

The historical GCC 3.3.3 compiler and GNU as 2.15 pass
**90/90, 69,688/69,688 exact named original bytes**.
The generator's independent host tests reject corrupted original RTTI
name bytes, invalid type-name encodings and nondeterministic
source/config contents. The underlying data gate fails if even one
object byte, original identity or section differs.

Final **whole-ELF** byte identity, including all original functions,
data, link order, section placement, headers and padding, is still
mandatory and unachieved. A passing name/RTTI/data gate is necessary
but not sufficient to establish original source or whole-program
binary reconstruction.

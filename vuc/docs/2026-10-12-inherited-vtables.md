# VUC inherited virtual tables: authenticated ABI and exact data

## Original evidence

The original VUC Linux i386 ELF contains **67** class vtables of exactly
12 bytes: **44 for IR classes** and **23 for AST classes**. Each has
three words: a zero offset-to-top, a pointer to that class's
authenticated C++ type-information object, and one inherited
`virtual` method pointer.

The virtual method pointer for each table resolves to the original
defined, named `.text` function, one of:

- `ASTNode::apa()`
- `IR::Type_Data::zzzzMakeRTTIWork()`
- `IR::Operator_Data::zzzzMakeRTTIWork()`
- `IR::Expr_Data::zzzzMakeRTTIWork()`
- `IR::UnOperator_Data::zzzzMakeRTTIWork()`
- `IR::Stmt_Data::zzzzMakeRTTIWork()`
- `IR::Fragment_Data::zzzzMakeRTTIWork()`
- `IR::Module_Data::zzzzMakeRTTIWork()`

These inherited methods' five-byte no-op implementations are already
separately accepted by the strict function gate. The inherited method
selection follows the **72 actual single-inheritance links** previously
derived from the original ELF typeinfo objects. No source-language
guessing is used to choose parent classes or relocation targets.

## Reproducible source-backed data matching

`vuc/scripts/vtable_inventory.py` reads the canonical
ZIP-backed ELF, verifies every original vtable symbol name/address/size,
checks the three original words, verifies the RTTI address and virtual
method symbol against the original symbol table, walks the original
RTTI ancestry to confirm method inheritance, and generates:

- `vuc/src/IRVtableProbes.cpp`, with ordinary historical C++
  declarations for the 44 IR class vtables;
- `vuc/src/ASTVtableProbes.cpp`, with ordinary historical C++
  declarations for the 23 AST class vtables;
- 67 independently address-specific
  `vuc/config/data_vtable_*.json` strict data manifests.

The C++ source includes **synthetic static instances solely to cause
GCC 3.3.3 to emit inherited virtual tables**. These instances are not
claimed to have existed in the original executable, do not qualify
as accepted original application functions, and must not be blindly
linked into a purported final byte-identical VUC executable.

Historical GCC 3.3.3 (Jan 25, 2004) emits **all 67 vtables** with
the exact original 12-byte size, and the strict independent
data verifier confirms all **804/804 bytes** and **134 original
RTTI/virtual-function relocations**.

No assembly snippets, original linked pointer constants, injected
binary data, or artificial code padding are used to force equality.

## Aggregated named-data acceptance scope

Together with the original initialized-buffer, RTTI name, RTTI
typeinfo, and integrated 52-byte `yyFlexLexer` vtable milestones:

| Explicitly checked object group | Objects | Bytes |
| --- | ---: | ---: |
| Previously accepted initialized objects and RTTI | 171 | 70,624 |
| `FlexLexer` and `yyFlexLexer` vtables | 2 | 92 |
| Inherited 12-byte IR and AST vtables | 67 | 804 |
| **Total** | **240/240 exact** | **71,520/71,520 exact** |

The strict named-data gate resolves **390 authenticated relocations**,
and the 90 host regression tests pass. These are verified named object
matches, **not a complete original section-layout or whole-ELF score**.
The entire original executable must ultimately match byte for byte.

## Reproduce

```sh
python3 vuc/scripts/vtable_inventory.py --write
python3 -m unittest discover -s vuc/tests -q

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-all-vtables-oct12
```

Generator tests reject corrupted original RTTI and virtual-method
pointers, invalid ancestry, absent original virtual method symbols,
non-reproducible configurations, and unsupported ELF layout.

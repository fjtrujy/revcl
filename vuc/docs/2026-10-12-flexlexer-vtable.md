# Byte-exact recovery of the original yyFlexLexer vtable

The canonical VUC 2004.1 ELF contains a **52-byte** Itanium C++ ABI
vtable for `yyFlexLexer` at `0x08074c80` and a **40-byte**
base-class `FlexLexer` vtable at `0x08075be0`. The actual byte-exact
acceptance gate now reproduces this vtable using the same ordinary C++
class and virtual destructor source that previously gave **78/78 bytes**
for each of the complete and deleting destructor implementations.

| Original vtable entry offset | Authenticated original reference |
| --- | --- |
| 0x00 | offset-to-top zero |
| 0x04 | `typeinfo for yyFlexLexer` |
| 0x08 | complete destructor |
| 0x0c | deleting destructor |
| 0x10 | `yy_switch_to_buffer` |
| 0x14 | `yy_create_buffer` |
| 0x18 | `yy_delete_buffer` |
| 0x1c | `yyrestart` |
| 0x20 | `yylex` |
| 0x24 | `switch_streams` |
| 0x28 | `LexerInput` |
| 0x2c | `LexerOutput` |
| 0x30 | `LexerError` |

The original vtable has **12 independently resolved ELF32 relocations**:
one for the RTTI and 11 for virtual function pointers. The original
vtable type information at `0x08075bc4` and the vtable's initial four
zero bytes also match. Both destructor functions remain exact after
adding the full virtual method declarations to
`vuc/src/FlexLexerDestructors.cpp`.

The base-class vtable contains six pure virtual method slots,
each relocated to the original `__cxa_pure_virtual@@CXXABI_1.2`
entry at `0x0804964c`, plus original RTTI and both base destructor
entrypoints. Its **40/40 bytes** and **nine relocations** match the
historical compiler. Defining these six methods as pure virtuals
keeps the already exact derived 52-byte vtable and both derived
78-byte destructor variants byte-exact in the *same* C++ source.

The original virtual function ordering is now authenticated by
independent data and code checks. This improves on earlier incomplete
class declarations that represented only the destructor and buffer
deletion method.

To independently validate the object:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-yyflex-vtable-oct12
```

The strict named-data gate includes
`vuc/config/data_yyflexlexer_vtable_exact.json` and
`vuc/config/data_flex_base_vtable_exact.json`.
The direct destructor checks remain in the exact function manifest.

This is **92/92 exact named data bytes**, not a full executable byte
match. The rest of the C++ lexer type layout and the compiler's
complete original translation unit are still only partly recovered.

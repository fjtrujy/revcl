# VUC byte-exact application-code batch: lookup, printers and conversions

Date: 2026-10-10. Branch: `vuc`.

## Mandatory exact comparison

Using the date-matched GCC **3.3.3 20040125 prerelease** compiler
and GNU **as 2.15** in the existing Linux VM, this batch added
**15 independently checked application function-symbol records** to
the exact-only gate. The accepted set now passes **95/95 function
records, 4,420/4,420 original instruction bytes**, with
**199 resolved ELF32 relocations** and **32 verified literal bytes**.

| Original function | Address | Exact instruction bytes |
| --- | --- | ---: |
| `MLHashTable<MLGameString, MLGameString, ...>::clear()` | `0x08050ca0` | 64/64 |
| `IsCommutative(IR::Operator_Data*)` | `0x08050c50` | 66/66 |
| `InstructionList::Add(Instruction*)` | `0x08051530` | 55/55 |
| `Print(ASTNodeList<ASTExpr*>&,...)` | `0x0805cba0` | 80/80 |
| `Print(ASTNodeList<ASTStmt*>&,...)` | `0x0805cbf0` | 80/80 |
| `Print(ASTNodeList<ASTVarDecl*>&,...)` | `0x0805cc40` | 80/80 |
| `Print(ASTNodeList<ASTDecl*>&,...)` | `0x0805cc90` | 80/80 |
| `ConvVal(MLGameString const&,float&)` | `0x0804f5d0` | 51/51 |
| `ConvVal(MLGameString const&,int&)` | `0x0804f610` | 60/60 |
| `ConvVal(MLGameString const&,unsigned&)` | `0x0804f650` | 60/60 |
| `yyFlexLexer::LexerError(char const*)` | `0x08061930` | 54/54 |
| `MLHashTable<MLGameString,SymbolTableEntry*,...>::FindCopy()` | `0x0806a8e0` | 114/114 |
| `MLHashTable<MLGameString,MLGameString,...>::FindPtr()` | `0x08050ba0` | 114/114 |
| `AlignConstant(char const*)` | `0x08049c20` | 69/69 |
| `yyFlexLexer::yy_switch_to_buffer()` | `0x08061730` | 75/75 |

The 1,102 new accepted instruction bytes are part of named original
functions only. **Whole-executable byte equality has NOT been achieved.**

## Reconstructed class and template behavior

The `MLHashTable` template has a 513-element pointer bucket array
and 16-byte nodes (`key` at offset 0, cached `hash` at 4,
`value` at 8, `next` at 12). `FindCopy` computes the hash,
selects a bucket, walks the list, compares cached hashes and then
invokes `MLHashFun<MLGameString>::IsEqual`; a match returns the
value. `FindPtr` performs the same traversal but returns a pointer
to the node's value field, at byte offset 8. The recovered ordinary
C++ template yields **114/114 exact bytes for each**.

The `MLHashFun<MLGameString>` specialization declaration is now
shared through `MLHashFunGameString.h` instead of being local to
`MLHashTable.cpp`, enabling consistent template instantiation.
Previously accepted string hash, equality and lookup functions
remain exact under the suite.

`InstructionList::Add()` is precisely `std::vector<Instruction*>::push_back`
compiled against GCC 3.3's libstdc++ header layout. Removing an
incorrect provisional four-byte field from the `InstructionList`
class declaration fixed its actual vector offset. With
`-falign-jumps=1`, the ordinary vector expansion matches all
55 original bytes, including the relocation to its non-inlined
`_M_insert_aux` helper. The helper's own implementation and the
original class header remain to be recovered.

## AST printer argument evaluation

All four original `Print(ASTNodeList<T>&,ostream&,separator)` functions
iterate over `ASTNodeList<T>`'s three-field layout, print each element,
and insert the separator after every element except the last.
The separator overload resolves to the original C++ iostream PLT
entry.

For `ASTExpr*`, `ASTStmt*` and `ASTDecl*`, the source form

```cpp
T *entries = list.entries;
Print(entries[i], output);
```

matches the original order of evaluating/storing call arguments.
The `ASTVarDecl*` specialization instead requires

```cpp
T item = list.entries[i];
Print(item, output);
```

to reproduce all 80 bytes. Both are ordinary C++: there is no
handwritten assembly, source-embedded instruction buffer or injected
padding. The distinct provisional files document an unresolved
**original translation-unit and source-template organization issue**;
they do not assert this is how the historical code was partitioned.

## Numeric conversion and lexer failure

All three `ConvVal` overloads return true after parsing the
`MLGameString::c_str()` value. The float overload writes the float
conversion; the signed and unsigned overloads use decimal base 10.
The linked VUC ELF calls `__strtod_internal` and `__strtol_internal`
through their original PLT entries. The accepted C++ fragment
explicitly calls those ABI entrypoints with the zero grouping flag.
This reproduces the original bytes, but the original source spelling
may have used historical glibc macro/inlined `strtod`/`strtol`.
The exact ABI call, not that source spelling, is established.

`yyFlexLexer::LexerError()` performs
`std::cerr << text << '\\n'; std::exit(2);`, with the original
`std::cerr` object address and three original PLT targets.
GCC reproduces the full 54-byte body without changing the previously
accepted `FlexLexer` destructors or `LexerOutput`.

`IsCommutative()` tests the original operator type values
`13, 11, 7, 6, 1, 0, 9, 8`. Writing seven early true returns and
a final equality-to-8 return preserves the original branch structure.
A unified boolean chain was non-exact; the recovered early-return
structure matches all 66 bytes.

## Constant text alignment and lexer buffer state

`AlignConstant(char const*)` calls the original `AlignString()`
with the global `constantsTabStop` at `0x0808aac4`, finds the
`"0x"` marker in the returned buffer and rewrites its two bytes
to `H'`. This ordinary source is **69/69 bytes exact**, with four
resolved relocations and the original three-byte literal verified.
The longer `AlignString()` algorithm is still experimental:
its recovered static `formatted[256]` symbol is authentic at
`0x0808aae0`, but current C++ compiles to 142 bytes rather than
the original 145.

`yyFlexLexer::yy_switch_to_buffer()` is **75/75 bytes exact**.
The recovered lexer member offsets are `0x28` (active buffer),
`0x2c` (held character), `0x30` (character count), `0x34`
(input cursor) and `0x40` (switch-on-EOF flag). It saves pending
buffer state, selects a new buffer, invokes the existing
`yy_load_buffer_state()` implementation and marks that flag.
The exact compiler comparison revealed an operand-order clue:
`if (current_ == next)` matches the original `cmp` register
encoding; `if (next == current_)` differs by one byte.

The related `yy_delete_buffer()` hypothesis currently reaches
**85/87 bytes**, differing only in the register selected for the
ownership flag test, and remains unaccepted. Both lexer probes
retain historical `free()` and buffer-state behavior without
invented machine-code sequences.

## Still being reconstructed

The original `MLGameStringTable::Get()` uses 513 bucket zeroings,
a static guard, two trailing scalar fields, and a destructor
registration. Its exact original `gst` and guard object names
and addresses are now checked against the original BSS symbol table.
The current plausible source hypothesis reaches **94/106**
instruction bytes in its best tested normal C++ form,
but still differs in the sequence that zeros the trailing fields and
loads the `__dso_handle` pointer; it is **not accepted**.

The `MLTempStringData::VFormatString()` candidate models its 0x1fbf
buffer limit, output cursor, two `vsprintf` attempts and historical
assertion messages/line numbers. The current plain C++ hypothesis
compiles to **236 bytes versus 238 original**, and has not yet
reproduced the original literal-section placement. It remains
experimental and is not counted.

`IsAccumulatorable()` also has a plausible conditional helper
reconstruction, but the tested 77-byte variant only matches 61
instruction bytes, so it remains a scratch probe. The previous
`MLTempString` constructor pair and `Printer_UglyPrint` remain
non-exact as documented earlier.

## Reproduce the accepted set

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-95
```

Expected: **95/95 exact**, **4,420/4,420 instruction bytes**.
The original ELF still has **195 unresolved text-function records**
and **8 experimental records** out of 298 total; these are function
counts, **not** a whole-binary match percentage. No complete-ELF
byte-equality claim is made.

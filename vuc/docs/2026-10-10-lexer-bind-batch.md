# VUC additional exact functions: lexer, binding and translation

## Accepted results

Historical GCC **3.3.3 (20040125 prerelease)** compiled with archived GNU
**as 2.15** now validates **44/44 explicitly accepted function-symbol
records** against the original ZIP-backed VUC executable: **2,002/2,002
instruction bytes**, **59 resolved relocations**, and **29 per-manifest
checked literal bytes**. This adds 10 complete matching function records
since the prior 34-function milestone. The whole ELF remains unmatched.

| Newly reconstructed symbol | Original address | Exact bytes | Relocations |
| --- | --- | ---: | ---: |
| `FlexLexer::~FlexLexer()` D1 | `0x080619a0` | 14/14 | 1 |
| `FlexLexer::~FlexLexer()` D0 | `0x080619b0` | 18/18 | 2 |
| `Translate(ASTNodeList<ASTVarDecl*>&)` | `0x0806e070` | 45/45 | 1 |
| `Bind(ASTNodeList<ASTVarDecl*>&,SymbolTable&)` | `0x0804edd0` | 57/57 | 1 |
| `Bind(ASTNodeList<ASTExpr*>&,SymbolTable&)` | `0x0804ed50` | 57/57 | 1 |
| `Bind(ASTNodeList<ASTStmt*>&,SymbolTable&)` | `0x0804ed90` | 57/57 | 1 |
| `Bind(ASTNodeList<ASTDecl*>&,SymbolTable&)` | `0x0804f0a0` | 57/57 | 1 |
| `MLHashTable<MLGameString,SymbolTableEntry*,...>::clear()` | `0x0804f020` | 64/64 | 1 |
| `Bind(ASTNodeList<MLGameString>&,SymbolTable&)` | `0x0804f060` | 57/57 | 1 |
| `script_lex()` | `0x0806fa50` | 39/39 | 3 |

All 10 are *ordinary C++* function bodies or compiler-generated ABI
entrypoints, not artificial instruction sequences or injected padding.

## Destructor ABI and vtable

The two `FlexLexer` virtual destructor entries are produced by GCC's
standard Itanium C++ destructor variants (`D1` complete destructor,
`D0` deleting destructor). The original ELF retains the `_ZTV9FlexLexer`
vtable symbol at `0x08075be0`, and GCC emits the expected vtable
address-point relocation with an addend of 8. The deleting destructor
then tail-calls `operator delete(void*)` at `0x0804973c`.

The accepted `vuc/src/FlexLexer.cpp` fragment passes both destructor
checks *and* the previously recovered `yyFlexLexer::LexerOutput()`.
This does not authenticate the full original `FlexLexer` class,
virtual method ordering, complete vtable contents or original source
file boundaries.

## Why two equivalent Bind forms are byte-different

The original list templates use the 12-byte `ASTNodeList<T>` layout
from the semantic-analysis batch. For `ASTVarDecl*`, the following
ordinary C++ expression gives the exact register/stack ordering:

```cpp
T element = list.entries[i];
Bind(element, symbols);
```

For `ASTExpr*`, `ASTStmt*`, `ASTDecl*`, and `MLGameString`, that
form changes eight bytes of the calling sequence. A separate C++
source expression yields full equality:

```cpp
T *entries = list.entries;
Bind(entries[i], symbols);
```

The equivalent-looking forms change the point where GCC stores
`symbols` into the outgoing stack argument area relative to the
entries-pointer load. The `MLGameString` instantiation uses address
arithmetic to pass the element as a reference rather than dereferencing
it as a pointer.

The accepted fragments `ListDispatch.cpp` (variable declaration)
and `BindLists.cpp` (four others) explicitly retain the different
forms. This provisional separation **does not claim that they were
separate original translation units**. The reproducible source
sweep is in `vuc/probes/evaluate_bind_variants.py`.

The translated variable-declaration list uses the same three-field
layout and a loop with a call to original `Translate(ASTDecl*)` at
`0x0806ce20`. All 45 instruction bytes reproduce with historical
jump alignment.

The additional `MLHashTable<MLGameString,SymbolTableEntry*,...>::clear()`
is instantiated from the existing reconstructed template and matches
the original 64-byte body without changes to either previously accepted
hash-table cleanup variant.

## Recovering the script_lex return type

The original wrapper loads the global `lexer` at `0x0808bcc0`,
indirectly invokes virtual slot at vtable offset `0x18`, reloads the
lexer object's `line` field at offset `0x0c`, and stores the value
into the global `script_line` at `0x0808bcc4`.

The first C++ hypothesis used `void script_lex()` and could reproduce
only 36/39 bytes: three register-selection bytes differed after the
virtual call. The original code *retains EAX* through the call and
field-copy sequence. Writing `int script_lex()` with the token returned
by the virtual `yylex()` method makes the compiler preserve that
value and reproduces **39/39 bytes**. This also reveals the intended
behavior of the wrapper. The temporary `ScriptLexer` class remains an
ABI-level hypothesis, not a reconstructed full lexer definition.

## Non-exact investigations

The `yyFlexLexer::yy_load_buffer_state()` hypothesis identifies the
original field offsets but currently yields 37 bytes instead of 39.
The original preserves `EBX`, whereas the provisional C++ source uses
caller-saved registers; the experimental source stays outside the gate.

`MLGameStringTable::Get()`, `AsmOutput_WritePrologue()`, the paired
formatted `MLTempString` constructors, and `Printer_UglyPrint()`
remain experimentally reconstructed, not accepted. The other **249**
function-symbol records remain unresolved.

## Reproduction

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-44
```

Expected: 44/44 exact, 2002/2002 code bytes, 59 relocations.
The experimental manifest is intentionally non-exact. No whole-binary
score is asserted. The mandatory final acceptance target remains an
**entire executable matching the original byte for byte**.

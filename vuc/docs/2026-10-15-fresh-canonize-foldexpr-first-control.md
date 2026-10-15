# New VUC Canonize_FoldExpr reconstruction: initial negative control

Date: 2026-10-15. Original VUC 2004.1 Linux i386 executable.
Original function `Canonize_FoldExpr(IR::Expr_Data*)` at
`0x080502c0`, **410 instruction bytes**. It was previously
UNRESOLVED without an original-length selected source candidate.

## Reverse-engineered original behavior

The original disassembly reveals the following real control flow:

1. If `expr->kind != 6`, set original global `error`
   (`0x0808bccc`) to 1 and write the original diagnostic literal
   `"illegal operand in const expr!"` from `0x08072460`
   to `std::cout` (`0x0808a7a0`), then return the original
   expression pointer.
2. For `kind == 6`, inspect its child at offset +12. If the
   child is a named constant (`kind == 7`), allocate a 12-byte
   `IR::ConstLit_Data` (original vtable
   `_ZTVN2IR13ConstLit_DataE`, original object kind 7), copy
   its `MLGameString` at offset +8, allocate a 16-byte
   `IR::ConstExpr_Data` (original vtable
   `_ZTVN2IR14ConstExpr_DataE`, kind 6), and install
   the existing unary operator at offset +8 and new child at
   offset +12.
3. If the inner expression is a binary operator (`kind == 5`),
   recursively fold and update its two expression children at
   offsets +12/+16. Continue only if `IsFoldable` returns
   true and both folded operands are constant-expression
   wrappers of named constants.
4. Use the newly reconstructed `ConcatenateBinOp` with
   the original binary operator and both nested `MLGameString`
   objects to create an operator expression string, then wrap
   the string in a new `ConstLit_Data` and `ConstExpr_Data`.
   Return the original expression if no reduction is possible.

Authentic named vtable symbols and original ELF addresses were
read from the canonical binary, not guessed.

## First ordinary C++03 source

`vuc/probes/CanonizeFoldExprNewOct15p.cpp` reconstructs this
behavior using the original `IR` class hierarchy and real C++03
virtual class data, plus ordinary `new`, recursive calls and
`std::cout`. This initial source is deliberately **not selected**
or accepted. Historical GCC 3.3.3 Jan25 compiled it to:

| Flags | Candidate length | Original |
| --- | ---: | ---: |
| `-O2` | 384 | 410 |
| `-O2 -frename-registers` | 381 | 410 |
| `-O2 -frename-registers -falign-jumps=1` | 368 | 410 |
| `-O2 -frename-registers -falign-jumps=8` | 373 | 410 |

None passes the mandatory original-function-length gate. We
therefore **do not claim original-offset byte coverage** or
promote this source to an experimental manifest. The first
disassembly comparison shows that the original expression
constructor writes the kind field before the vtable pointer,
while this simplistic virtual base class initializes its vptr
first. The error diagnostic store and `std::cout` argument
setup also have different instruction scheduling.

The remaining work is to reconstruct authentic original
`IR::ConstLit_Data` and `IR::ConstExpr_Data` base/derived
constructor layouts, and genuine Canonize.cpp translation-unit
predecessors, rather than manually ordering virtual-pointer
stores. Do not add fabricated vtable addresses or handwritten
assembly in place of C++ class construction.

Generated compiler objects live under
`/tmp/vuc-binutils215/canonize-fold-new-*-oct15p.o`
in the existing Lima guest.

The initial template reconstruction remains
**204 exact / 32 experimental / 62 unresolved** across
the whole 298-function inventory, with `Canonize_FoldExpr`
still unresolved. Whole-ELF byte identity is not established.

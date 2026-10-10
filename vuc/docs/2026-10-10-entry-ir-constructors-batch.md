# VUC byte-exact batch: entry point, AST checks, instruction/fragment construction

Date: 2026-10-10. Branch: `vuc`.

## Exact acceptance

Using the recovered January 25, 2004 GCC 3.3.3 source compiler
and GNU as 2.15, the strict original-ELF matcher now passes
**112/112 accepted function-symbol records**, **5,724/5,724
original instruction bytes**, **239 resolved ELF32 relocations**
and **35 original referenced literal bytes**. The host suite
passes **50 tests**. The whole executable is **not byte-exact**.

| Reconstructed function | Original address | Exact bytes |
| --- | --- | --- |
| `InstructionList::Print(std::ostream&)` | `0x080518a0` | 87/87 |
| `Check<ASTExpr*>(ASTNodeList<ASTExpr*>&)` | `0x0806a110` | 66/66 |
| `main(int,char**)` | `0x08071530` | 63/63 |
| `__tcf_1` (AST arena cleanup) | `0x0804a8f0` | 46/46 |
| `InstructionList::AddL(MLGameString const&)` | `0x080516b0` | 107/107 |
| `TranslateFrag(MLGameString const&)` | `0x0806dd30` | 87/87 |

This batch adds **456 byte-exact instruction bytes** compared with
the previously pushed 106-function checkpoint.

## InstructionList vector printing and label insertion

`Print()` traverses `std::vector<Instruction*>` with the
original GCC 3.3 normal-iterator layout, calls the already exact
`Instruction::ToString()`, and writes that string followed by
`std::endl`. It emits **87 original instruction bytes** and
four verified relocations. The existing `Add(Instruction*)`
continues to match exactly after integrating `Print()`.

`AddL()` revealed the original `Instruction` size of 12 bytes,
its `MLGameString` at offset 0, a four-byte intermediate field,
and its kind/status field at offset 8. Calling
`Add(new Instruction(name))` with a constructor that copies the
string and writes kind 2 yields **107/107 bytes** including GCC's
original exception-unwind path, all five relocations, and the
custom `Instruction::operator new(unsigned)` target.
The related `Add(char const*,...)` format overload remains
experimental. Its current 168-byte candidate differs from the
original 146-byte construction order: the original allocates
before constructing its temporary MLGameString.

## Check list returning nontrivial Type

Unlike the three previously accepted list-check instantiations,
`Check(ASTExpr*)` returns a 16-byte `Type` value. The C++03 ABI
passes a hidden return-object pointer to the per-expression call.
Correctly declaring the 16-byte result type reproduces the
extra stack frame, temporary object address and four-byte stack
adjustment. Its 66-byte specialization is exact in
`vuc/src/CheckExprList.cpp`.

The separate fragment is provisional: adding that instantiation to
the three prior list checks changes the historical compiler output
for the other instances. No guessed historical translation-unit
boundaries have been accepted as fact.

## VUC executable entry point

`main()` invokes `ParseArguments(argc,argv)` and returns -1 on
failure. Otherwise, it invokes `ParseFile()` and returns -2 when
the original one-byte `error` flag at `0x0808bccc` is set, or
0 on success. The original **63/63** instruction bytes and all
three relocation targets require *explicit early returns*, not
the superficially equivalent ternary-return expression (60/63).

## AST arena cleanup

`ASTNode::mem` at `0x0808abf8` stores the head of a linked
series of 64-KiB arena chunks. A generated static destructor
walks the chunks, reads the predecessor at offset `0x10004`,
deallocates the old chunk and updates the head. Normal C++
destructor code under historical GCC emits the original
46-byte `__tcf_1` exactly, including three relocations.
This is compiler-generated lifecycle code, not an independent
algorithmic function.

The corresponding `Instruction::instrData` cleanup at
`0x08051980`, with 8-KiB chunks and predecessor at `0x2004`,
is **45/46** bytes under all tested normal C++ loop variants;
its last scratch-register pop differs. It remains experimental.

## IR fragment construction

`TranslateFrag()` allocates 12 bytes, initializes the base
kind/status field at offset 4, stores the derived fragment's vptr,
and copy-constructs the `MLGameString` at offset 8. A
**nonvirtual base class** (containing the kind) and a derived
class with a virtual destructor reproduce the original store
ordering. The resulting new-expression generates the exact
87-byte body, five verified relocations, and the two original
exception-cleanup calls. Declaring all fields directly in the
derived polymorphic class changed the two stores' order and
was non-exact.

These provisional source declarations are an ABI-consistent
reconstruction; the original C++ headers have not been recovered.

## Remaining mandatory work

The exhaustive **298 original text-function** inventory has
**112 EXACT**, **8 EXPERIMENTAL** and **178 UNRESOLVED** records.
No whole-ELF match score is available. Exact executable linkage,
startup runtime, all data sections and authentic historical
library/toolchain inputs remain mandatory.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-112
```

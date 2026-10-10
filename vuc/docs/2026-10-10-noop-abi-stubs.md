# Original five-byte VUC no-op ABI hooks

Date: 2026-10-10. Branch: `vuc`.

The original ELF contains ten named, non-padding, size-five functions
whose entire body is `push %ebp; mov %esp,%ebp; pop %ebp; ret`.
Ordinary C++ empty method definitions compiled by the recovered
January 25 2004 GCC 3.3.3 prerelease and GNU as 2.15 reproduce
**5/5 instruction bytes each**. These are actual compiler-generated
no-op methods, not arbitrary linker filler or fabricated assembly.

The seven IR `zzzzMakeRTTIWork()` methods are:
`Type_Data`, `Operator_Data`, `Expr_Data`, `UnOperator_Data`,
`Stmt_Data`, `Fragment_Data`, and `Module_Data`.
The other three original methods are `ASTNode::apa()`,
`InstructionList::GenerateImage(unsigned char*)`, and
`Value::Set(ASTLValExpr const*)`.

The accepted fragments are `vuc/src/IRRTTIDummies.cpp` and
`vuc/src/EmptyHooks.cpp`. Each manifest names the actual original
virtual address and original mangled symbol. No relocation or
literal rebasing is necessary for these ten no-op bodies.

This should not be presented as ten recovered application algorithms.
It is faithful original symbol coverage and ABI reconstruction, while
the original source header/class layouts remain provisional.

The strict candidate gate is now **132/132 exact function-symbol records,
6,644/6,644 code bytes**, **287 resolved relocations** and **36
verified referenced literal bytes**. The full inventory contains
132 EXACT, 10 EXPERIMENTAL and 156 UNRESOLVED out of the original
298 function-symbol records. The 51 host tests pass. The original
complete VUC ELF is **not yet byte-identical** and no whole-binary
match percentage is asserted.

Reproduction:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/gate-132
```

# VUC InstructionList line-comment and scope-copy reconstruction probes

Date: 2026-10-12; branch `vuc`.

## Authoritative checkpoint

The handed-over 106-function count is historical. The tracked exact manifest
already includes `InstructionList::Print()` (original `0x080518a0`, 87/87
instruction bytes), `AddL()`, `GenTmp()` and `GenLabel()`. The recovered
January 2004 GCC 3.3.3 / GNU as 2.15 gate independently revalidated
**166/166 accepted function records**, **11,314/11,314 instruction bytes**,
with **417 supported relocations**. The host suite passes 119 tests.
The full VUC executable has **not** been rebuilt byte-identically.

## `InstructionList::AddLineComment(int)`

Original function: `_ZN15InstructionList14AddLineCommentEi` at
`0x08051720`, **162 bytes**. The disassembly supports the following ordinary
C++ algorithm:

- Calculate the size of `std::vector<Instruction*>`, check the final
  instruction's kind (3) and line number, and return if the line comment
  is already present.
- Otherwise allocate a 12-byte `Instruction`, initialize an `MLGameString`
  from the original empty literal, copy it into the allocated instruction,
  set kind 3 and the original source line, then call `InstructionList::Add()`.
- Preserve C++ exception cleanup through `operator delete` and
  `_Unwind_Resume`.

`vuc/probes/instruction_add_line_comment_oct12.cpp` expresses this without
inline assembly, binary patches, dummy padding or hardcoded function VAs.
`vuc/config/instruction_add_line_comment_oct12_probe.json` verifies all
seven original relocations and the one-byte NUL literal.

The best normal C++/compiler hypothesis currently reproduces the **162-byte
length** with **132/162 matching instruction bytes**, **7 resolved
relocations**, and **1/1 checked literal byte**, using
`-O2 -frename-registers -falign-jumps=4`. This is **not** an accepted
function. Compared with the original, the compiler reverses the zero argument
and empty-literal setup around `MLGameString` construction and places some
alignment bytes at different basic-block boundaries. Do not manufacture
instruction padding to hide those differences.

`evaluate_add_line_comment_oct12.py` tests 60 combinations: four plausible
`Instruction` constructor forms, three natural vector-access forms, and
five GCC 3.3 jump alignments. None was exact. The original's indexed
vector access is more faithful than a `.back()`-based reconstruction:
it yields the same pre-allocation indexing sequence.

## Historical STL scope-copy instantiation

Original function at `0x0804f220`, **120 bytes**:
`std::__uninitialized_copy_aux<Scope*, Scope*>()`, with
`Scope = MLHashTable<MLGameString, SymbolTableEntry*, ..., 513>`.

The historical C++ library emits the expected `0x804` stride, copy
constructor call, exception handler and scope cleanup calls. With
`-O2 -frename-registers`, however, the candidate generated from
`vuc/probes/scope_uninitialized_copy.cpp` is **127 bytes**, not 120.
The original increments the cleanup iterator before calling `clear()`;
the recovered compiler's candidate calls `clear()` before incrementing.
Its loop/exception landing-pad alignment also differs.

`evaluate_scope_uninitialized_copy_oct12.py` tests 40 combinations of
supported GCC 3.3 register-renaming, loop, jump and label alignments.
None produces an exact original-sized 120-byte function; all results remain
experimental. Revisit only with new provenance for the historical STL
instantiation or the original translation-unit context.

## Additional checked candidates

The existing `evaluate_accumulatorable.py` forms produced a best **61/77**
matching bytes for `IsAccumulatorable()` at `0x0805a570`; no tested form
was exact. The provisional `Translate(ASTNodeList<ASTStmt*>&)` at
`0x0806ada0` compiles to 116 versus the original 119 bytes, and the
provisional `Type::IsAssignable()` at `0x0806a1c0` compiles to 186 versus
151. These remain in `vuc/probes/`, not `vuc/src/` or the exact manifest.

## Reproduction

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-oct12-resume
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_add_line_comment_oct12.py
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_scope_uninitialized_copy_oct12.py
```

The separate exact initialized-data and `.bss` gates must be retained,
but neither their matches nor source-layout declarations count as extra
function instruction bytes. The 298-symbol queue remains **166 EXACT,
9 EXPERIMENTAL, 123 UNRESOLVED**. Completing the ELF still requires
all source units, linker and runtime provenance, section ordering, and
byte-identical linked output against the canonical ZIP member.

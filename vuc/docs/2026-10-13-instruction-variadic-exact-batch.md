# VUC InstructionList formatted Add and AddB: two exact application functions

Date: 2026-10-13. Branch: `vuc`.

## New 146-byte matching functions

| Original method | ELF address | Instruction bytes | Relocations |
| --- | --- | ---: | ---: |
| `InstructionList::Add(char const*, ...)` | `0x08051570` | **146/146** | 7/7 |
| `InstructionList::AddB(char const*, ...)` | `0x08051610` | **146/146** | 7/7 |

Both functions are genuine application code compiled from ordinary C++
inside the existing `vuc/src/InstructionList.cpp` source-unit fragment.
The historical January 25, 2004 GCC 3.3.3 and GNU assembler 2.15
produce all original instruction bytes under `-O2 -frename-registers`.
The candidate implementations are identical except for the original
stored instruction kind (`0` for `Add`, `1` for `AddB`).

The recovered source performs the actual original operations:

1. Allocate an automatic `char line[1024]`, initialize `va_list`, and
   format into it via `vsprintf(line, format, arguments)`.
2. Invoke the original 12-byte `Instruction::operator new(unsigned)`.
3. Construct a temporary `MLGameString(line, false)`, then
   copy-construct the instruction's `MLGameString` member; GCC 3.3.3
   emits the original stack offsets `-0x408` for text and `-0x418`
   for the temporary.
4. Store instruction kind to the original field at offset `+8` and
   invoke the original `InstructionList::Add(Instruction*)`.
5. Preserve the original `operator delete(void*)` / `_Unwind_Resume`
   exception cleanup path rather than guessing a hand-written handler.

All seven original ELF32 relocations in each method resolve to:
`vsprintf` at `0x0804961c`, `Instruction::operator new` at
`0x08051910`, original `MLGameString` char-pointer and copy
constructors at `0x08071720` and `0x08071770`, the list's pointer
`Add` at `0x08051530`, `operator delete` at `0x0804973c`, and
`_Unwind_Resume` at `0x0804998c`. Neither method requires an original
string-literal mapping. No instruction patching, forced register asm,
dummy functions, section-offset guesses, or changes to original ELF
bytes are used.

Adding these methods to the existing `InstructionList.cpp` fragment
retains exactness of previously accepted `GenLabel()` **85/85**,
`GenTmp()` **85/85**, and `Print()` **87/87** methods in the same
compiler object. The pointer-taking `InstructionList::Add()` remains
independently accepted and is the genuine callee for the two new
methods. The ordinary C++ source-level hypothesis was originally
recorded and compared as
`vuc/probes/instruction_varargs_add_oct13.cpp`.

## Follow-up: line comment source

The neighboring `InstructionList::AddLineComment(int)` at
`0x08051720`, original size 162, has the same exception cleanup and
string construction. A faithful source fragment in
`vuc/probes/instruction_line_with_unit_oct13.cpp` detects whether the
last instruction already carries kind `3` and the same line number,
then creates an empty-string comment instruction and stores its line
at offset `+4`. With genuine preceding InstructionList methods and
four-byte jump alignment the candidate emits 162 bytes, all seven
named relocations, and the original 22 literal bytes in its combined
source unit. The function initially matches **147/162** bytes, but
its alternate short branch target and NOP scheduling around the
line-duplicate early return remain unresolved; this is not an exact
function and is not promoted. The bounded genuine C++ branch-form
script `vuc/probes/evaluate_instruction_line_context_oct13.py` is
preserved for continued investigation.

An additional **54-configuration** historical GCC 3.3.3 alignment
matrix in `vuc/probes/evaluate_instruction_line_alignment_oct13.py`
tests 2-, 4- and 8-byte jump alignment, 1-, 2- and 4-byte label/loop
alignment, and normal versus register-renaming builds. The best
authenticated comparison remains **147/162** with four-byte jumps,
one-byte labels and register renaming. Other options either move more
instructions or produce an incompatible original-function length
(most notably 170 bytes with eight-byte jump alignment). This
reinforces that exactness probably depends on the original control-flow
block frequencies or translation-unit context, not on arbitrary
padding/alignment flags. The experimental candidate remains unaccepted.

## Acceptance

With the other three complete application functions in this batch,
strict original-function coverage reaches **187/298**,
**12,575/12,575 instruction bytes**, eight experimental and 103
unresolved named functions. The untouched original archive remains
authoritative. A whole linked VUC ELF byte-for-byte rebuild is **not**
yet established.

## Additional nonexact string formatting source evidence

The original `MLTempString::Set(char const*, ...)` at `0x08071d00`
has a 117-byte function body. A normal C++03 varargs implementation
which formats into the existing original static string arena and
returns its owning `MLTempString*` compiles to **117 bytes**, with
**111/117 matching instruction bytes**, all **eight original ELF
relocations** mapped, and no unverified literals under plain `-O2`.
The remaining six differences are register assignment choices in the
two format-argument setup instructions and `memset` initializer.

`vuc/probes/evaluate_mltemp_set_context_oct13.py` tests source order
with real `MLTempStringData::FormatString()` and the formatted
`MLTempString` constructor. Additionally,
`vuc/probes/evaluate_mltemp_set_receivers_oct13.py` evaluates 13 genuine
`this` aliases, format/data pointers, and temporary result-lifetime
formulations under four GCC options. None improves the 111/117 result
or satisfies the strict exact gate. The supported 111/117 result is
tracked in `vuc/config/ml_temp_set_probe.json` as **experimental only**;
the original 117-byte function is not promoted.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-instruction-add-187
```

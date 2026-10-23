# VUC ReduceBinOp signed/unsigned: two new 315/315 exact functions

2026-10-23; original immutable VUC 2004.1 ELF32-i386 archive; branch
`vuc`. Historical tools: recovered GCC 3.3.3 prereleases of
2004-01-25 and 2003-12-06, and GNU as 2.15 (the actual original Debian
compiler binaries are not authenticated).

## What changed

The two independently tracked C++03 template bodies were **300/315**
in both specializations. The differences were original offsets
`+60` and `+258..+271`: the compiler aligned the **division**
branch when the original had aligned the **multiplication** branch.
Everything else, including type-correct signed `idiv` versus
unsigned `div`, already matched original bytes.

A bounded single-condition study
`vuc/probes/evaluate_reduce_binop_real_branch_probabilities_oct23.py`
compiled **28 source variations × both compiler snapshots =
56 historical builds**. It annotated each of the 14 real opcode guards
in turn with `__builtin_expect(operation == opcode, 0 or 1)`,
leaving the original operator cases, output assignments, defaults and
signed/unsigned template types unchanged. Only **opcode 10 (division),
predicted unlikely** recovered the *original* function length,
the exact landing-payload alignment and all 315 original-offset bytes
for **both** template specializations:

| Function | Original ELF VA | Old bytes | New bytes |
| --- | --- | --- | --- |
| `ReduceBinOp<unsigned>` | `0x08050540` | 300/315 | **315/315** |
| `ReduceBinOp<int>` | `0x08050680` | 300/315 | **315/315** |

Compiler flags stay `-O2 -frename-registers -falign-jumps=8`.
Both date-matched GCC snapshots independently produce
**315/315**, **zero relocations** (the original body has none), and
zero compiler-literal dependencies. No ASM, byte blobs, NOP injection,
linker offset fabrication, artificial calls or register constraints are
used.

The relevant source is now
`vuc/src/ReduceBinOp.cpp`:

```cpp
else if (__builtin_expect(operation == 10, 0)) out = lhs / rhs;
```

This expression is an ordinary GCC 3.3 C++ source-level branch
prediction hint; the Boolean truth value, division behavior and all
observable source-level semantics remain unchanged. The match
establishes that the emitted original x86 code is reproduced, **not**
that this exact spelling of the source survived from 2004. It is a
plausible original-era compiler hint, and is retained as a clearly
documented code-generation hypothesis, not claimed as archival source.

## Acceptance and manifests

The original partial manifest file names are preserved to maintain
stable references:
`vuc/config/reduce_binop_unsigned_partial_oct15o.json` and
`vuc/config/reduce_binop_signed_partial_oct15o.json`.
Their source now points to accepted `vuc/src/ReduceBinOp.cpp`, and
both manifests are appended to
`vuc/config/exact_candidates.json`. They already existed in
`vuc/config/selected_candidates.json`; no duplicate is introduced.
They are removed from the partial regression floor manifest, since
**315/315** now belongs solely to exact acceptance.

The original experimental candidate
`vuc/probes/ReduceBinOpNewOct15o.cpp` remains intact for
forensic comparison, and the full 28-case source/object scores
are in VM scratch
`/tmp/vuc-reduce-binop-guard-control-oct23/results.json`.

The accepted named-function inventory is now
**207 EXACT / 37 EXPERIMENTAL / 54 UNRESOLVED**.
Exact code verification grows from **16,093** to **16,723**
original-offset checked instruction bytes; both are still
candidate-scoped, not whole-ELF coverage.
The 36 remaining verified original-length partial checkpoints
retain their original relocation/literal verification and regression
floors; the independent 37th selected experiment is wrong-sized
`SymbolTable::Enter` and is not scored.

## Follow-on near-exact cleanup control

The 46-byte original `Instruction.cpp` static arena destructor
`__tcf_1` remains **45/46**. Eight natural C++03 source controls in
`vuc/probes/evaluate_instruction_arena_real_branch_prediction_oct23.py`
test a real `while`, `for`, and early-break loop with
`__builtin_expect` on the original nullable arena pointer. All
controls retain the real arena cleanup behavior. The original-sized
ones preserve the **same single dead scratch-register difference at
offset +42**; some versions produce 47–60-byte bodies and are
ineligible as partial matches. These results are negative controls,
not grounds to manipulate a register, insert a NOP, or promote the
destructor to exact acceptance.

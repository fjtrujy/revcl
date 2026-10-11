# Byte-exact VUC AST function-call semantic checking

Date: 2026-10-11. Branch: `vuc`.

## Accepted source-level reconstruction

`Check(ASTFunCall*)` at `0x080623f0` now matches the canonical
2004.1 ELF's **118/118 instruction bytes**, with all **three
original ELF32 relocations** independently resolved, using
January 25, 2004 GCC 3.3.3 and GNU as 2.15
(`-O2 -frename-registers`).

The ordinary C++03 source in `vuc/src/CheckASTFunCall.cpp`:

- calls `Check(ASTLValExpr*)` on the callee pointer stored at AST
  function-call offset `+0x08`, producing a discarded 16-byte
  result on the stack;
- checks the 12-byte argument list at `ASTFunCall + 0x0c` via
  `Check<ASTExpr*>()`;
- accesses the callee's function-signature pointer from
  `ASTLValExpr + 0x20`;
- calls `CheckActualParamList()` with the argument list, the
  function-signature parameter vector at `+0x24`, and the
  associated name at `+0x20`;
- then **reloads** the callee and signature pointer and returns
  the first 16-byte `Type` value from the signature.

The explicit original linked call addresses are `0x080687a0`,
`0x0806a110`, and `0x08061a20`. The provisional AST and
function-signature member names are reconstructed labels rather
than original source declarations.

## Source-level finding: implicit C++ copy

Initially, the exploratory `Type` had a user-defined four-word
copy constructor. The generated function was 118 bytes long and
matched **100/118** bytes: the final four field loads and stores
were scheduled in a different order, although the same values
were copied.

Removing the explicit copy constructor permits the historical
compiler to generate the **implicit C++03 copy constructor**.
The original code's consecutive load/store pairs are reproduced,
and the entire function becomes **118/118 exact** without any
inline assembly, machine-code blobs or artificial padding.
Both ordinary implicit copying and an explicit
`__builtin_memcpy(..., 16)` also match in the diagnostic
sweep; the implicit constructor is preferred because it requires
fewer ungrounded source constructs.

The source-order insight is equally important: using one local
`FunctionSignature* signature` over the parameter validation call
prevented GCC from reloading the pointer and produced a different
120-byte function. Reaccessing the callee/signature fields on each
side of the potentially mutating call matches the original memory
reads, aliasing assumptions and code length.

The reproducible C++03 source and ABI alternatives remain in
`vuc/probes/check_ast_fun_call.cpp` and
`vuc/probes/evaluate_check_fun_copy.py`.
Only the accepted source under `vuc/src/` is included in the
strict exact manifest.

## Verified coverage and limits

The resulting strict gate is **155/155 original named function
records**, **9,372/9,372 instruction bytes**, with **402
resolved relocations** and **132 per-manifest verified literal
bytes**. Original text-function records: **155 EXACT,
9 EXPERIMENTAL, 134 UNRESOLVED** out of 298. The 60 host
tests pass. The pending arithmetic and compiler register-allocation
probes have not been accepted.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/resume-155
```

**Whole-executable byte equality remains mandatory and unachieved.**
This milestone checks one complete named function from an independently
identified portion of the ELF, not a fraction of total executable bytes.

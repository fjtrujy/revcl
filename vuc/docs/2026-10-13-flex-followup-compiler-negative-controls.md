# VUC: bounded Flex ownership, predecessor, and optimizer controls

Date: 2026-10-13. Branch: `vuc`, based on rebased `main`
`a0b6acf`. Prior accepted commit: `9e4582c`.

## Result

**No new byte-exact function and no higher-scoring partial
were recovered in this batch.** It is important not to confuse equal
compiled length or a genuine source-level experiment with new exact
byte coverage.

The previously recovered
`yyFlexLexer::yy_get_next_buffer()` at `0x080611c0` remains
**430/550** matching instruction bytes, generated from
`vuc/probes/yy_get_next_buffer_partial_oct13.cpp` using
`-O2 -frename-registers -falign-jumps=1`. All four relocations
(one authentic `realloc` relocation plus the three original
fatal-error strings) and the current 152-byte verified candidate
literal section remain authentic as specified in
`vuc/config/yy_get_next_buffer_oct13_probe.json`.

The literal section includes 23 compiler alignment zeros which happen
to match mapped original zero bytes; this mapping does **not** prove
historical translation-unit literal placement. It remains experimental
and unpromoted.

## New reproducible negative controls

Every experiment compiles ordinary C++03 through the recovered
GCC 3.3.3 historical toolchain and invokes the strict original ELF
function checker **only when the produced function has the original
required byte length**. No generated object, original bytes, assembly,
synthetic helpers, padding or linker relocations were patched.

- `vuc/probes/evaluate_yy_next_ownership_oct13.py`:
  tests natural forms for buffer-offset calculation, reassignment,
  remaining input capacity, growth computation, fatal-error
  checks, saved old-buffer pointer and `realloc` result, across
  `-falign-jumps=1` and 2. The 550-byte variants continue to match
  **430/550**, and no better source form is identified.
- `vuc/probes/evaluate_yy_next_predecessors_oct13.py`:
  restores genuine previously recovered `LexerInput()` and
  `LexerOutput()` source bodies in all five meaningful source-unit
  prefix/order cases under four optimization configurations.
  The original lexical predecessors do **not** change the 430/550
  match. This is specific to the tested bodies; it does not
  prove the original source file boundary or peephole history.
- `vuc/probes/evaluate_yy_next_optimizer_oct13.py`:
  two authentic GCC 3.3.3 prerelease compiler snapshots,
  seventeen bounded period optimizer/alias/branch/peephole switches.
  Both snapshots agree at **430/550** under accepted flags.
  Disabling second scheduling still gives the original **550**
  code bytes but regresses the match to **296/550**;
  several other options change the function size and cannot
  qualify for strict original-length comparison.
- `vuc/probes/evaluate_scope_copy_alignment_oct13.py`:
  natural historical libstdc++ `__uninitialized_copy_aux`
  for 513-bucket `MLHashTable` pointers, both standard-template
  instantiation and genuinely exception-cleaning source.
  Across six jump and four label alignments the original **120-byte**
  function length was not reproduced. Nearest sizes include
  119, 123, 127 and 135. Do not register misleading 119/120
  as a byte-matching score.
- `vuc/probes/evaluate_scope_iterator_align_next.py`:
  tests the real C++ STL `SymbolScope` iterator instantiation
  under 60 jump/label/loop alignment combinations. The target is
  **149** bytes but none emits that original length; representative
  candidate lengths are 145, 147, 151 and 159.

## Reason to avoid another blind flag sweep

The original `yy_get_next_buffer` disassembly has a 4-byte alignment
NOP immediately before the buffer-growth loop at relative `+0x160`.
The best historical compilation places a related growth block
at `+0x15c` and allocates a different set of scratch registers
for the retained buffer pointer, scanner `this` pointer and owner
flag. The upstream VCL compiler work notes that genuine
translation-unit provenance and compiler peephole state can cause
exactly this kind of mismatch. But VCL GCC 3.2.2 register effects
cannot simply be transplanted into VUC GCC 3.3.3.

The tested predecessor and optimizer controls did not alter the
register choices; they ruled out simple spellings, not all
source-unit contexts. Future work should reconstruct the original
generated Flex scanner compilation unit or inspect GCC RTL dumps
around loop scheduling and `realloc`, rather than accumulating
additional arbitrary optimization flags.

## Preservation and acceptance

The accepted VUC inventory remains **194/298 named functions**
(**13,847/13,847 strict instruction bytes**), with
**16 experimental**, **88 unresolved**. The previously checked
initialized data has **244/244** objects (**71,536/71,536 bytes**)
and BSS has **12/12** original declarations. Whole linked ELF
identity remains **unestablished**.

These controls are retained as negative evidence and reproducible
future starting points. They make no promotion to
`vuc/config/exact_candidates.json`,
`vuc/config/selected_candidates.json`, or `vuc/src/`.

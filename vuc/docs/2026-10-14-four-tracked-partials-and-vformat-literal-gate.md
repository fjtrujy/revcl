# VUC byte-exact reconstruction: newly tracked genuine partials and literal-provenance gate

**Date:** 2026-10-14. **Branch:** `vuc`. **Initial HEAD:** `a8afdb4`.
**Original oracle:** unmodified archived VUC 2004.1 ELF32.
**Compiler:** restored January 25, 2004 prerelease GCC 3.3.3 with GNU as 2.15
in the existing Lima guest `revcl-oracle`.

## Results

No new complete byte-exact function was obtained. Four previously
`UNRESOLVED` original functions now have **independently recompiled,
original-length, strict relocation-verified partials** recorded as
`EXPERIMENTAL`:

| Original function | Address | Matched code bytes | Authenticated relocation sites | Experiment |
| --- | --- | ---: | ---: | --- |
| `Value::operator+() const` | `0x0806e9b0` | **172/219** | 1 | `value_addition_partial_oct14_probe.json` |
| `AssembleBinOp(...)` | `0x0805a3f0` | **162/193** | 4 | `assemble_binop_dispatch_oct14_probe.json` |
| `Translate(ASTNodeList<ASTExpr*>&)` | `0x0806ae20` | **90/256** | 2 | `translate_expr_list_oct14_probe.json` |
| `Canonize_Reduce(IR::Fragment_Data*)` | `0x08050460` | **78/224** | 11 | `canonize_fragment_oct14_probe.json` |
| **Total, distinct functions** | | **502/892** | **18** | |

All four candidates return `exact:false` from the *same* VUC original
ELF matcher used by accepted functions. The addition function uses a
small diagnostic C++ source wrapper to include the existing
`ValueArithmeticUnit.cpp` with real +, -, * method predecessors, but
does not modify that accepted C++ source. Its `TypeConvert` call
relocation is verified at its actual original address.

Other selected sources were already committed in previous experiments,
but the four functions had never been entered into the managed
experimental manifest. This batch **integrates and gates** them,
rather than falsely claiming the previously observed raw byte scores
were discovered today. They do not count toward new *exact* coverage.
The AST expression-list candidate uses a normal pointer-copy loop
where the original uses `rep movsl`; this is a diagnostic partial,
**not a proven authentic original source transcription**.
Likewise `AssembleBinOp` and fragment reduction use unproven period
GCC `__builtin_expect` hypotheses for their code layouts.
No assembly instruction, fake padding or original relocation address
was synthesized.

The VUC function ledger advances from **197 EXACT / 15 EXPERIMENTAL /
86 UNRESOLVED** to **197 EXACT / 19 EXPERIMENTAL / 82 UNRESOLVED**.
No accepted C++ implementation, accepted manifest, original archive,
root VCL project, or previous VUC compiler snapshot was changed.

## New VFormatString original-string evidence and negative experiment

A separate new source-level experiment,
`vuc/probes/evaluate_vformat_authenticated_literals_oct14.py`,
verifies the original ELF bytes for all **five distinct diagnostic
string literals** used by
`MLTempStringData::VFormatString(const char*, char*)`
(original `0x08071a50`, **238 bytes**):

| Verified original address | Original literal |
| --- | --- |
| `0x08076860` | `char* MLTempStringData::VFormatString(const char*, char*)` |
| `0x080768a0` | `char* MLTempStringData::Reserve(int)` |
| `0x080768c5` | `numWritten >= 0` |
| `0x080768d5` | `../megalib/MLGameString.cpp` |
| `0x080768f1` | `Max() > strLen` |

It tested **260 historical GCC compile recipes**, using genuine
`Reserve` bounds, reset, retry, temporary lifetime and conditional
control-flow variations. Two produced the original **238-byte**
function size with all **10 relocation sites** independently
resolved to original named calls or byte-authenticated literals.

**Critical rejection:** the generated candidate's `.rodata` also
has **32 bytes of compiler alignment gaps**. The 154 bytes of
non-padding literals are authenticated, but those 32 gap bytes have
no proven original translation-unit address. The strict checker
therefore *correctly rejects both original-length candidates* as
`candidate literal section contains unmapped bytes`. A diagnostic
relocation-resolved code comparison, *without incorrectly certifying
the whole emitted literal section*, was only **32/238** and
**24/238**, respectively. Neither candidate is entered into
`selected_candidates.json`.

This explicitly fixes an important prior investigative blind spot
without weakening acceptance or inventing an arbitrary address for
alignment zeros. Future VFormat work should recover the genuine
`MLTempStringData` translation-unit constant emission order.

## Independent scanner control

A re-run of `evaluate_get_next_word_switch_registers_oct14.py`
checked **240 historical source/flag combinations** with authentic
scanner globals. The best original-sized candidate is **8/135**
code bytes, with genuine original relocations, not a useful enough
partial for the experiment manifest. No accepted candidate resulted.

## Acceptance policy and verification

The four sources and configs were either already committed or are new
tracked diagnostic-only files. The repository tests now ensure these
four original-sized candidates are in
`vuc/config/selected_candidates.json` but **never in**
`vuc/config/exact_candidates.json`.

Reproduce the added partial, for example:

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
  -falign-jumps=4 -c "$R/probes/value_addition_partial_oct14.cpp" \
  -o /tmp/vuc-binutils215/value-add-verified-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/value-add-verified-oct14.o \
  --config "$R/config/value_addition_partial_oct14_probe.json"
```

The established comprehensive historical acceptance gates remain
mandatory: 197/197 exact functions and 14,417/14,417 accepted
instruction bytes; 244/244 initialized data objects and
71,536/71,536 initialized bytes; 12/12 BSS declarations with
18,437 source allocation bytes; host unit tests; and
`git diff --check`. A passing suite verifies *function-level*
reconstruction, **not linked whole-ELF byte equivalence**.

All 163 pre-existing untracked paths, especially the extracted
original VUC/VCL archives and unrelated local work, are preserved.
No new worktree, subagent, source mutation of root VCL, invented
machine bytes or unverified relocation targets are permitted.

## Next candidate family

For more **new partial bytes**, attack source-unit ownership and
IR data constructor lifetime within the 224-byte fragment reducer
and the 256-byte AST expression-list translator. Both reproduce
their genuine original external call graph but still diverge in
register lifetimes and output array copying. For new **exact**
functions, concentrate on a genuinely different original small
function or original compiler patches, rather than repeating the
exhausted alignment/expect permutations on accumulator/printer
near matches.

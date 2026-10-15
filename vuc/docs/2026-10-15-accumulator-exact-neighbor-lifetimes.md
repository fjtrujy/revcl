# VUC follow-up: register-lifetime controls after 77-byte exact accumulator

**Date:** 2026-10-15. **Branch:** `vuc`.
**Working tree:** original connected Mac at `/path/to/revcl`,
no extra worktrees or subagents. Archived VUC 2004.1 Linux i386
binary remains untouched.

## Byte-exact checkpoint

This batch recovered the previously experimental
`IsAccumulatorable(IR::Operator_Data*)` as **77/77 original-size
machine-code bytes** with zero relocations at `0x0805a570`.

New exact source: `vuc/src/IRAccumulatorableExact.cpp`; authenticated
in both recovered GCC 3.3.3 prerelease snapshots and GNU as 2.15.
Previously it was **74/77**. Source-level negative condition with
ordinary C++03 `goto next_case11` reproduces the original conditional
branch orientation. No inline assembly, register pinning, fake NOP
padding or byte patching.

Accepted inventory becomes **201 EXACT / 24 EXPERIMENTAL /
73 UNRESOLVED** of 298 original symbols and **15,007** exact original
instruction bytes. The exact suite compiled 201/201 functions and
matched 15,007/15,007 bytes; initialized data and BSS suites remain
244/244 objects and 12/12 declarations. `vuc/tests/` passes
168 host tests. No claim is made about whole-ELF identity.

Detailed reconstruction and reproduction steps:
`vuc/docs/2026-10-15-accumulator-77-byte-exact-negative-goto.md`.

## Natural C++03 lifetime controls: MLTempString::Set

Function `_ZN12MLTempString3SetEPKcz` at `0x08071d00`, original
size **117**. The existing selected candidate
`vuc/probes/ml_temp_set_return.cpp` matches **111/117** and resolves
all **8 original relocation sites**, including the static
`MLTempStringData::Get()` singleton guard / BSS, `memset` and
`VFormatString`.

The independently reproducible driver
`vuc/probes/evaluate_mltemp_set_source_ownership_oct15d.py`
compiled **1,404** C++03 candidate recipes in the historical Linux
guest: 13 natural inline singleton constructor variants × 6 real
`Get()` pointer/reference-lifetime variants × 3 varargs setter
lifetime choices × 6 compiler flag combinations.

- **1,170 original-length, relocation-checked objects**.
- **234 compile or strict-validation failures**.
- Best original-sized overlap remained **111/117**. No new exact or
  improved experimental match; original function flags and source
  remain selected.

All compiler generated source and object files, plus the full JSON
result ledger, remain in the guest under
`/tmp/vuc-binutils215/mltemp-set-source-ownership-oct15d/`.
The driver itself is checked into `vuc/probes/`.
The observed 6 differing bytes are still GCC allocator choices
around the saved formatter string pointer and `memset` fill-size
arguments, not proof of differences in original functionality.

## Natural C++03 stream/literal lifetimes: script_error

Function `_Z12script_errorPKc` at `0x0806fa80`, original
size **161**, previously selected strict original-sized partial
**155/161** with **15 authenticated relocations** and
**5 verified string-literal bytes**.

The source driver
`vuc/probes/evaluate_script_error_register_lifetimes_oct15d.py`
models genuine iostream expression chaining, output stream aliases,
local literal lifetimes, message pointer lifetimes, and seven
historical GCC optimizer settings. Its full declarative matrix
contains 14 source statements × 6 independent lifetime controls ×
7 compiler settings, **588 candidate recipes**.

The recovered iostream template compilation is slow. This bounded
batch compiled **288** distinct object files before intentionally
ending its own diagnostic compiler process rather than leaving
work running after the response.

- The separate source-level validation checkpoint
  `vuc/probes/summarize_script_error_partial_oct15d.py`
  reports **52 original-length and fully relocation/literal-checked
  candidate objects**, with **236 invalid under the strict gate**.
- The best eligible candidate remains **155/161**.
- **No new exact match was found in the completed portion** of the
  search; the remaining 300 declarative combinations were not
  evaluated in this batch and are not claimed as negative results.
- The evaluated score ledger lives in the guest
  `/tmp/vuc-binutils215/vuc-script-error-lifetimes-oct15d/partial_results.json`.
  The source is intentionally left in `vuc/probes/`; no exact or
  selected manifest change is justified.

This batch does not attempt to force a register allocation using
inline assembly, GCC hard-register bindings or invented local
effects. Wrong-size bodies or ungrounded literal relocations are not
promoted as partials.

## Full regression / limits

The new accepted source was matched independently with the January
and December GCC 3.3.3 snapshots. Full candidate function/data/BSS
checks use the original archived binary and do not modify it.

- `201/201` exact machine-code functions;
  `15,007/15,007` original bytes.
- `244/244` initialized data objects;
  `71,536/71,536` original bytes.
- `12/12` BSS declarations;
  `18,437` original source allocation bytes.
- `168` host unit tests passed.

All **155 unrelated visible untracked paths** from the latest
`origin/main` checkout were left intact; 8 historical `.DS_Store`
files became ignored through the other main branch's pre-existing
`.gitignore` update, not by deletion. No changes to the root VCL
reconstruction.

Next effective actions are to trace actual source translation-unit
ownership for the remaining scratch-register differences, or move to
a distinct unresolved method with original DWARF and relocation
evidence. Blind reiteration of the completed 1,404 `Set`
variants or 288 evaluated iostream candidates is unlikely to help.

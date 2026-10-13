# VUC AlignString: exact source-context recovery after VisitCache main rebase

Date: 2026-10-13. Branch `vuc`. Original VUC 2004.1 Linux x86
binary remains read-only in the verified upstream ZIP.

## Rebase and technique learned

Fetched `origin/main` at **`ea1221c`**, which adds the original
VCL 1.4 beta 7 `VisitCache.cpp` matching milestone and
`config/matching-evidence/visitcache-owner-split.md`.

Updated local `main` and rebased **103** existing VUC commits
without conflict, from `57434e1` onto `ea1221c`, producing
`f7f44aa` before this batch's changes. The 146 pre-existing untracked
paths and all tracked VUC content were preserved.

Main's new `VisitCache` evidence shows ordinary
`#pragma implementation` ownership and source-unit splitting can
change dead scratch-register choices in GCC's peephole2 and renaming
passes, even when genuine C++ bodies are unchanged. VCL's original
compiler is GCC 3.2.2; VUC's historical GCC 3.3.3 compiler is
different, so the technique is not transferable as an unverified
binary or provenance assertion.

## Newly byte-exact original function

`AlignString(char const *, int)` at original **`0x08049b80`**,
exact **145/145** original bytes.

Source: `vuc/src/AlignString.cpp`.
Accepted manifest: `vuc/config/align_string_exact_oct13.json`.
Historical compiler: recovered Jan 25, 2004 GCC 3.3.3, GNU as 2.15,
`-O2 -frename-registers -falign-jumps=4`.

The original **eight relocation sites** resolve to the genuine
`strcpy` PLT target `0x08049a0c`, `strchr` at `0x0804962c`,
`strcat` at `0x0804989c`, and the actual
`AlignString::formatted` 256-byte BSS object at **`0x0808aae0`**.
This method references no initialized literal section; the accepted
minimal source has no fabricated literal mappings.

The formerly selected experimental contextual source
`vuc/probes/align_string_real_unit_oct13.cpp` matched **143/145**,
with a compiler-chosen `strchr` scratch register difference at
offsets 41 and 47. The unchanged real function placed **before**
the other original `AsmOutput` methods emitted **145/145** in
`vuc/probes/evaluate_align_source_order_oct13.py`. No artificial
neighboring functions, dead variables, NOPs, binary patching or
assembler constraints were used.

A second experiment,
`vuc/probes/evaluate_align_standalone_oct13.py`, then verified that
the true isolated C++03 function with just `#include <cstring>`
also emits **145/145**, reproducing the original symbol size and all
relocations under **both** GCC 3.3.3 prerelease snapshots
(Dec 6, 2003 and Jan 25, 2004) using `-O2 -frename-registers`
with `-falign-jumps=1` or `=4`. Under normal `-O2` without
`-frename-registers` the candidate is **144** bytes and is
rejected. The accepted function source is the minimal genuine
implementation, not a 5-method synthetic linking unit.

This proves **function-level** exactness and original static BSS
ownership, **not** the original author's file partition,
compiler date, Makefile flags, unobserved source spelling, or
a byte-identical linked whole ELF. The other formerly accepted
`AsmOutput` functions remain owned by their independent exact
sources and manifests.

## Bounded remaining experiments

- `vuc/probes/evaluate_instruction_owner_context_oct13.py`
  reconstructs genuine `Instruction`/arena owner declarations in
  separate headers. It tests `#pragma interface` and deferred
  `#pragma implementation` with the correct GCC syntax, alongside
  normal separation, `-fno-weak` and function sections, on both
  recovered GCC dates. All original-sized cleanup alternatives
  remain **45/46**, differing only in the original final dead
  scratch POP at offset 42. The VisitCache technique does not
  resolve this VUC target under tested real owners.
- `vuc/probes/evaluate_yy_next_loop_cfg_oct13.py`
  tests natural `while`/`for`/`do`/early-exit loop spellings for
  the genuine 550-byte `yy_get_next_buffer()` partial. All matching
  550-byte forms remain **430/550**; no new source is promoted.
- `vuc/probes/evaluate_translate_stmt_align_oct13.py` tests genuine
  AST statement-list translation and real `IR::Seq_Data`
  constructor semantics under bounded jump/label/loop alignments.
  No candidate emits the original **119-byte** length, so no
  misleading partial byte score is recorded.

These are reproducible negative controls, kept out of
`vuc/src` and the strict exact manifest.

## Reproduction

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  C=/tmp/vuc-binutils215/g++-jan25-as215
  "$C" -O2 -frename-registers -falign-jumps=4 \
    -c "$R/src/AlignString.cpp" \
    -o /tmp/vuc-binutils215/align-exact-accepted.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/align-exact-accepted.o \
    --config "$R/config/align_string_exact_oct13.json"
'
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_align_source_order_oct13.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_align_standalone_oct13.py"
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
```

The strict regression gate now contains **195/298** independently
accepted functions with **13,992/13,992** original instruction bytes;
the remaining inventory is **15 experimental** and
**88 unresolved**. The pre-existing 244 original data objects
(71,536 bytes) and 12 BSS declarations are checked by independent
strict historical-toolchain scripts. Whole ELF identity is not
established.

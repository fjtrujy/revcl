# VUC 2004.1 — byte-exact FlexLexer buffer allocation from its real predecessor

**Date:** 2026-10-15. **Branch:** `vuc`. **Target:** unchanged VUC
2004.1 Linux i386 ELF32 in the tracked upstream archive. **Compilers:**
historical GCC 3.3.3 Jan 25 2004 and Dec 6 2003 snapshots, both using GNU
binutils 2.15.

## Checkout, upstream and preservation

The original checkout had no staged or modified tracked files, and exactly
**155 pre-existing untracked leaf paths** (143 grouped untracked entries).
The initial HEAD was `2e82ae4`. The branch was fetched from `origin`
and cleanly rebased across **144** VUC commits onto new
`origin/main` **`63b12da`**; the rebased initial head was `054eeee`.
No worktree, subagent, reset or cleaning was used. All unrelated untracked
paths and original archives remain untouched. No root VCL code is modified.

The post-rebase baseline passed 173 host tests, the exact function gate
(201/201 functions, 15,007/15,007 bytes), initialized-data gate
(244/244 objects, 71,536 bytes), and BSS gate (12/12 declarations,
18,437 allocated bytes but zero file-backed bytes).

## Positive result: yyFlexLexer::yy_create_buffer at 139/139

Original symbol `_ZN11yyFlexLexer16yy_create_bufferEPSii`;
original ELF address **`0x080617b0`**, **139 instruction bytes**.

| Independent property | Previous selection | New reconstructed source |
| --- | ---: | ---: |
| Compiled original function size | 139/139 | **139/139** |
| Matching original instruction bytes | 131/139 | **139/139** |
| Differing original-byte offsets | 8 | **0** |
| Authentic function relocation sites | 5 | **5** |
| Verified string-literal bytes | 44 | **44** |
| Exact acceptance | NO | **YES** |
| Compiler snapshots | Jan/Dec GCC 3.3.3 | **Both verified** |

The former isolated source `vuc/probes/yy_create_buffer.cpp` already
generated the correct 139-byte algorithm and five authentic relocations,
but emitted eight wrong register-selection bytes at original function
offsets **65, 78, 102, 103, 109, 116, 121 and 135**.

The crucial missing source context was not the allocation-failure branch
spellings. The immediately preceding original method is
`yyFlexLexer::yyrestart(std::istream*)` at **`0x080616d0`**
(original length **93**). Compiling that **real method body immediately
before** the unchanged generated buffer allocator changes historical
GCC's register lifetime/renaming state. The very first new genuine
predecessor recipe turned **131/139** into **139/139**, without any
machine-byte manipulation, inline assembly, forced register variable,
synthetic pad or fabricated relocation.

The promoted historical source is
`vuc/src/FlexLexerCreateBufferUnit.cpp`, with period-plausible C++
and the previously authenticated yyFlexLexer virtual-method ordering.
The output preserves the original algorithm: allocate 0x28 bytes for
buffer state, fail through virtual `LexerError`, allocate `size + 2`
for bytes, store ownership, then `yy_init_buffer`. The preceding
`yyrestart` method still compiles to **93/93 byte-exact** code with
two authenticated relocation sites from the same object.

Both independently recovered historical compiler snapshots reproduced
**both** exact functions from the promoted unit; their outputs are
not merely same-length overlaps. The exact-only manifest now includes
`vuc/config/yy_create_buffer_probe.json` pointing into `vuc/src/`.
The earlier isolated source and exhausted error-guard matrix remain
intact as experimental history.

## Reproduction

First run the context search:

```sh
limactl shell revcl-oracle -- python3 \
  vuc/probes/evaluate_yycreate_true_prefix_oct15g.py
```

It saves the actual generated C++03 files, historical ELF32 objects,
and results JSON under
`/tmp/vuc-binutils215/yycreate-true-prefix-oct15g/` in the guest.
The single `yyrestart` predecessor is candidate number 005 and is
the first original-length **139/139** result; the isolated baseline is
candidate 000 at 131/139.

Independently compile the promoted source using *both* recovered
snapshots:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers \
    -c "$R/src/FlexLexerCreateBufferUnit.cpp" \
    -o "/tmp/vuc-binutils215/flex-create-real-${snap}.o"
  for cfg in yy_create_buffer_probe yy_restart_probe; do
    limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
      --object "/tmp/vuc-binutils215/flex-create-real-${snap}.o" \
      --config "$R/config/${cfg}.json"
  done
done
```

Each compiler produced 139/139 (five relocations and 44 literal bytes)
for `yy_create_buffer` and 93/93 (two relocations) for `yyrestart`.
The exact manifest compiles `yy_create_buffer` in its reconstructed
source context, rather than testing an isolated function alone.

## Other structural controls in this batch

`vuc/probes/evaluate_value_add_real_predecessors_oct15g.py`
compiled **70** historically plausible Value-addition source/compiler
recipes, adding the genuine `Set(ASTExpr*)` and/or
`Set(ASTLValExpr*)` predecessors (including the original immediate
predecessor), two vector-lane lexical orders and seven original-GCC
alignment/optimizer combinations. **45** retained original **219-byte**
length and one authentic TypeConvert relocation. None surpassed the
selected **203/219** original-offset partial, whose first exact prefix
is 169 bytes. The older experimental source remains unchanged.

The cleanup callback `__tcf_1` and other near-exact candidates must
still retain exact original length, relocations and any original
literal-byte constraints. Wrong-length and compilation-failed candidates
are negative controls, not accepted partial improvements.

`vuc/probes/evaluate_instruction_cleanup_original_list_prefix_oct15g.py`
also compiled **42** C++03 source/compiler recipes adding progressively
more genuine `InstructionList` predecessors in original ELF function order,
including the recovered variadic `Add` and `AddB`, `AddLineComment`,
`GenLabel`, `GenTmp` and `Print` bodies. **All 42** emitted the original
**46-byte** cleanup body and three authenticated relocations, but the
best remained **45/46**, with the same original-offset difference at
**+42** (`pop %eax` rather than the original `pop %ecx`). This is a
bounded *negative* compiler-state result, not a source promotion. The
reproducible sources, objects and results remain under
`/tmp/vuc-binutils215/instruction-list-prefix-oct15g/` in the guest.

## Acceptance and limits

The exact ledger advances from **201 EXACT / 25 EXPERIMENTAL /
72 UNRESOLVED** to **202 EXACT / 24 EXPERIMENTAL / 72 UNRESOLVED**
of 298 original named functions. Accepted code increases by precisely
**139 original instruction bytes**, from **15,007** to **15,146**.
Host tests have been updated to require the new accepted source and
classification. Initialized-data and BSS inventories are unchanged.

These are *function-scoped*, relocation-aware exact matches. There is
still **no independently verified byte-identical linked VUC ELF**;
translation-unit section layout, remaining code, complete linking and
library provenance remain open.

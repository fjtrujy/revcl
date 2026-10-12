# Size-first VUC byte-exact reconstruction

Date: 2026-10-12. Branch: `vuc`.

## Change in strategy

Work in **increasing original instruction-byte length**, using
`vuc/scripts/smallest_first.py`. This supersedes repeated exploratory
batches that arbitrarily select complicated candidate functions.

The canonical queue comes exclusively from original ZIP-backed ELF32
symbol identities and `exact_candidates.json` (never from candidate
lengths or inferred source extents). Within one size, order by original
virtual address. The queue shows original-size tiers, prior probe files,
and source-unit provenance when established by local STT_FILE symbols.

```sh
python3 vuc/scripts/smallest_first.py --max-size 64 --top 25
python3 vuc/scripts/smallest_first.py --top 40
python3 vuc/scripts/smallest_first.py --json /tmp/vuc-smallest-first.json
```

Use these rules:

1. Revalidate the existing exact-only gate before changing source.
2. Examine the smallest outstanding **positive-sized** symbol first.
   Zero-sized original symbols remain unverified (six in the present
   inventory); do not pretend that they are zero-byte exact functions.
3. Investigate the function's original bytes, DWARF, source-unit
   provenance, real relocation targets, and current normal-C++ probes.
4. Attempt a *bounded* set of grounded compiler/source hypotheses.
   Record the mismatch and move forward if no exact candidate emerges.
   Resume a blocked short function only with **new evidence**, not
   another arbitrary optimization permutation.
5. Promote only after **every** original instruction byte matches and
   all declared relocations and referenced literals resolve. Then
   rerun the entire strict regression gate and host tests.
6. Repeat the size-ordered queue after each promotion; do not count
   duplicate config files or data-only matches as new functions.
7. Continue into higher size tiers only when smaller items are exact
   or explicitly blocked. Being blocked does **not** mean acceptance.

No ordering policy can replace the mandatory final **whole-ELF
byte-for-byte** equality check.

## Baseline on October 12

The tracked original text-symbol inventory contains **298** named
function records, including six symbols of original size zero.
The current accepted gate has **166 EXACT**, **9 EXPERIMENTAL**, and
**123 UNRESOLVED** records. All **115** host unit tests passed before
this workflow change. These counts are symbol records, **not a
whole-binary matching percentage**.

The first positive-sized tier (up to 64 bytes) contains **17**
non-exact original functions:

| Original size | Count | Examples / status |
| ---: | ---: | --- |
| 30 | 1 | `AsmOutput_WritePrologue`, 22/30 (experimental) |
| 31 | 13 | original `_GLOBAL__I_*` wrappers; four previously probed at 29/31 |
| 39 | 1 | `yy_load_buffer_state`, previous 37-byte candidate |
| 40 | 1 | `Printer_UglyPrint`, previous 38/40 |
| 46 | 1 | `Instruction` arena `__tcf_1`, previous 45/46 |

For the 31-byte wrappers, several original source anchors are
still unresolved (e.g. `BindInlineAsm`, `ListFile::Generate` and
`yyFlexLexer::yylex`). Their compiler-generated source wrapper
cannot establish original translation-unit identity by itself.
`Value::Value()` is already exact, but the companion 31-byte
initializer wrapper is only 29/31 in the conventional full-source
probe. The queue displays whether the named global anchor is already
accepted; **this is a triage hint, not a proof of TU ownership**.

## Original-toolchain short-tier experiments

The **10 existing** config files associated with these 17 original
function identities were compared against the original ELF using the
January 2004 GCC compiler and GNU as 2.15 with
`recheck_pending_probes.py`. Results:

- `AsmOutput_WritePrologue`: 22/30, plus a separate unresolved
  original string-section relocation probe.
- Four startup wrappers (`matchStack`, `script_debug`,
  `currFileName`, `Value`): each 29/31. The `Value`
  name has two alternative source manifests, both 29/31.
- `Printer_UglyPrint`: 38/40.
- `Instruction` arena cleanup callback: 45/46; sole different
  byte is `pop %eax` versus original `pop %ecx`.
- `yy_load_buffer_state`: current candidate length 37 versus
  original 39.

These tests yielded **zero new accepted** short functions. They
remain blocked or exploratory, **not** advanced by relaxing the gate.

Additional checks using the authentic-source-date **December 2003
GCC 3.3.3** compiler produced **29/31** for the Value wrapper and
**45/46** for the Instruction callback, the same as the January
compiler. Compiling Instruction's cleanup together with the
reconstructed allocator also remained 45/46, so that specific
original-source-neighbor hypothesis is ruled out. These negative
results are preserved to prevent cycling indefinitely over the same
compiler configurations.

## Next strict queue

Use `--max-size 64` until the above symbol identities are resolved
or blocked with documented evidence. The next available tier,
`65..128`, begins with:

- `IsAccumulatorable()`: original 77 bytes.
- `__libc_csu_init`: 84 bytes (runtime provenance).
- `Canonize2_Reduce()`: original 85 bytes, best prior 83/85.
- `Print(ASTFunCall*)`: 98 bytes, best prior 96/98.
- `MLTempString()`: 101-byte constructor, experimentally mapped.
- `MLGameStringTable::Get()`: 106 bytes, partial source and
  original BSS relocation evidence.

Recheck existing diagnostics first and prioritize recovering
missing **original types, source neighbors or compiler provenance**
over indiscriminate register-flag sweeps. Promote none of the above
without 100% exact bytes and relocations.

## Reproduction

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/smallest_first.py --max-size 64 --top 25
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-size-first
```

The first command validates the host tests; the second is a planning
tool; the third must be a fully passing **existing exact-function**
gate. Neither the work queue nor a partial-match score claims any
whole-binary reconstruction success.

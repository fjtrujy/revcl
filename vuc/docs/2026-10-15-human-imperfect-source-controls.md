# VUC 2004.1 — human-written C++03 rules and fresh structural controls

Date: 2026-10-15. Branch: `vuc`. Target: unchanged, hash-pinned VUC 2004.1 Linux ELF32 i386; recovered Jan25 GCC 3.3.3 and GNU assembler 2.15 in the existing `revcl-oracle` VM. All experiments use ordinary C++03 source and the relocation-aware original ELF comparator. No inline assembly, register constraints, synthetic instruction bytes, fake relocations or alignment padding were used.

## Rules and preservation

The VUC-local `AGENTS.md` now explicitly treats the original program as human-written 2003-2004 C++ rather than optimized reference code. It permits evidence-driven redundant locals/stores/branches, signedness mistakes, old-style casts, unnecessary copies, strange loops, `goto`, fall-through and period compiler quirks. These are candidate *hypotheses*, not a reason to introduce matching-only filler. Inline assembly remains avoided except for an exceptionally justified original-era low-level operation documented before use. Exact function length, authentic relocations, literals and complete original byte equality still gate `vuc/src/` acceptance; original-sized partials stay in `vuc/probes/`.

At entry, `vuc` was at `f13ccfe` (already on `origin/vuc`); a separately staged prior-session Flex buffer report was preserved in commit `140752e`. Fetched `origin`; no new `origin/main` commits needed rebasing. The rules were committed in `2c69f6a`. The 155 pre-existing unrelated untracked files remain untouched, along with the original ZIP and root VCL reconstruction.

## New compiler/source experiments

The following checked-in drivers retain generated source and `.o` files and complete per-recipe `results.json` ledgers in independent `/tmp/vuc-binutils215/` directories inside the Lima guest. All candidate scores below are checked against the original ELF at the same function address, require the original function length and verified original relocations; where applicable they also require the original literal bytes. Invalid sizes and compile errors are never counted as partial progress.

| Driver under `vuc/probes/` | Recipes | Strict original-length candidates | Best strict score | Pre-existing selected score |
| --- | ---: | ---: | ---: | ---: |
| `evaluate_getnext_signed_leading_oct15h.py` | 176 | 68 | GetNextWord 50/135, 5 relocations | 50/135 |
| `evaluate_value_plus_alignment_interactions_oct15h.py` | 150 | 30 | Value addition 163/219, 1 relocation | **203/219** |
| `evaluate_printer_human_idioms_oct15h.py` | 98 | 84 | Print ASTFunCall 96/98, 7 relocations, 9 literal bytes | 96/98 |
| `evaluate_getnext_original_inlineasm_context_oct15h.py` | 20 | 10 | GetNextWord 50/135, 5 relocations | 50/135 |

**Total: 444 attempted historical source/flag combinations, 192 strict original-length, original-relocation candidates, no new byte-exact function or improved selected partial.** No manifest or accepted source was changed by these experiments.

### GetNextWord leading-character signedness

Original `GetNextWord` at `0x080511e0` does a signed `cmpb 10` / `jg` and `cmpb 9` / `jl` in its leading-whitespace loop. The selected 50/135 experimental source has different register ownership and unsigned comparison structure. The new driver tests real `char`, `signed char`, `unsigned char` and `int` input types with 11 explicit, occasionally redundant leading-whitespace branch spellings and four historically valid compiler control sets. Although 68 bodies preserved the 135-byte size and all five global-reference relocation sites, none improved the current 50/135 original-offset score. A matching signed predicate alone does not fix the preceding EAX/ECX ownership or code placement.

### Genuine InlineAsm.cpp predecessors

The original ELF orders `BindInlineAsm`, `GenInlineAsm`, and then `GetNextWord`. Source-unit context was decisive for the newly exact Flex lexer buffer allocator, so `evaluate_getnext_original_inlineasm_context_oct15h.py` tests existing authentic (but still partial) C++03 bodies for both InlineAsm predecessors before the selected scanner. This is not a manufactured compiler-state shim. Ten candidates retained 135-byte size and five authentic relocations; the best remained the isolated 50/135 source. The driver corrects an independent guest-scratch header path so the genuine predecessor combinations actually compile. The available predecessor bodies are not yet authentic matches, so their failure does not rule out true source-unit-state effects from the reconstructed original bodies.

### Value addition: assembler label placement

The original `Value::operator+` at `0x0806e9b0` has ascending x87 lane instructions and a scalar branch aligned at +0xd0. The selected strict partial reaches 203/219 after changing its genuine independent-lane lexical order. Tested 150 combinations of two C++03 lane orders and historically supported jump/label/loop alignment. Thirty retained original size and the TypeConvert relocation, but none beat the prior selected 203/219 source; the best among this bounded alignment grid was 163/219. Arbitrary assembler pads remain prohibited. Future attempts need source-level branch topology or historically real owner context, not more combinations of this same alignment grid.

### ASTFunCall printer: plausible inefficient C++

The original 98-byte function at `0x0805b770` differs from the selected source only at offsets +23 and +29: ECX in the original versus EAX in the candidate. Tested 98 combinations: genuine C++03 `register` locals, pointer-to-member access, old-style casts, redundant callee assignments, early argument aliases and original-era GCC optimizer controls. Eighty-four retained 98 bytes, all seven authentic relocations and nine original string-literal bytes. The best remained 96/98. These human-looking spellings alone do not reproduce the original register allocation. The existing accepted Printer_UglyPrint and iostream wrapper were not modified.

## Reproduction and next direction

```sh
R=/path/to/revcl/vuc
for driver in \
  evaluate_getnext_signed_leading_oct15h.py \
  evaluate_value_plus_alignment_interactions_oct15h.py \
  evaluate_printer_human_idioms_oct15h.py \
  evaluate_getnext_original_inlineasm_context_oct15h.py; do
  limactl shell revcl-oracle -- python3 "$R/probes/$driver"
done
```

Do not repeat this grid without a new concrete register-lifetime, source-unit or source-control-flow hypothesis. Potential next productive evidence is the original GCC 3.3 RTL live-range/peephole state around the first printer callee load, and more faithful original `BindInlineAsm`/`GenInlineAsm` bodies before further `GetNextWord` prefix tests. The old alignment and predicate variants are negative evidence, not production candidates.

## Regression acceptance

Host regression: 174 tests passing. Exact historical instruction suite: **202/202 functions, 15,146/15,146 identical instruction bytes**. Initialized data: **244/244 objects, 71,536/71,536 bytes**. BSS: **12/12 declarations, 18,437 allocated source bytes** (zero file-backed bytes). These are scoped checks, not byte-identity proof for the entire linked VUC executable.

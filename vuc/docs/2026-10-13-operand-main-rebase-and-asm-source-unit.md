# Rebase on VCL Operand.cpp and VUC AsmOutput source-unit reconstruction

Date: 2026-10-13. Branch: `vuc`.

## Rebase and recovered techniques

Fetched the current `origin/main` at `cd35047e7b9e86a32daf4c0e50bb2806dd094d6b`, "Complete Operand.cpp matching at 0x080566a8", and fast-forwarded the local `main`. Rebased **92 VUC-specific commits** cleanly onto it. The checkout was clean in tracked paths beforehand. All **134 previously untracked paths**, including original archives and interrupted scratch reconstruction probes, were preserved in place; no new worktrees, resets, cleaning, or subagents were used.

The new VCL historical reconstruction (`docs/matching-notes.md`, `src/Operand.cpp`, `README.md`) matches its 802-byte `Operand::Emit` under *GCC 3.2.2*, including its 224-byte switch jump table. It provides two concrete techniques, which must be evaluated independently before transferring to *GCC 3.3.3* / VUC:

- The precise `int` representation of a direct predicate, with `char` casts on comparison operands, avoids a Boolean normalization that changes the subsequent `cmp dl,dl` / `mov ecx,edx` sequence. Changing the predicate's source type can alter original instruction bytes even if its truth value is unchanged.
- Real switch-case family source ordering affects both the rnreg live-register assignments and peephole2 padding emissions. The original VCL physical block order is not an authoritative statement about lexical C++ source order. Reordering genuine cases 29/46, 31, and register-output families, along with a real surrounding source-unit prefix, corrected 802/802 without raw assembly manipulation.

The VCL repository's current whole-executable producer-union progress is separate from the VUC exact-gate counts. GCC 3.2.2 VCL flags, accepted author-specific exceptions and binary layout claims are **not** imported as claims about VUC.

## Stronger authenticated VUC AsmOutput.cpp compilation-unit evidence

The preceding VUC milestone accepted `AsmOutput_TabConstants(InstructionList&)` at `0x08049df0` **127/127** with seven original relocations, with `AsmOutput_WritePrologue`, `AsmOutput_WriteEpilogue` and `AlignString` compiled immediately before it in the real source order. This batch recovered the additional real original neighbor:

| VUC function in actual original order | Original ELF address | Original code size | Current combined-object matching |
| --- | --- | ---: | ---: |
| `AsmOutput_WritePrologue` | `0x08049b40` | 30 | 22/30, still partial |
| `AsmOutput_WriteEpilogue` | `0x08049b60` | 30 | 22/30 in this exact scanner unit; separately accepted 30/30 |
| `AlignString` | `0x08049b80` | 145 | **143/145**, now tracked experimentally |
| `AlignConstant` | `0x08049c20` | 69 | **69/69 exact**, preserved in the combined source unit |
| `AsmOutput_PrintInstruction` | `0x08049c70` | 382 | Not yet reconstructed here |
| `AsmOutput_TabConstants` | `0x08049df0` | 127 | **127/127 exact**, preserved in the combined source unit |

The committed `vuc/src/AsmOutputUnit.cpp` now includes original-style C++ functions `AlignString` and `AlignConstant` in that order, not a fabricated prefix. `AlignConstant` actually calls the original `AlignString` and `strstr`, then rewrites `0x` to `H'` when present. The historical compiler and the correct source alignment produce **69/69 exact `AlignConstant` with four ELF32 relocations** and **127/127 exact `TabConstants` with seven relocations**, simultaneously from the same genuine source object. The independent strict accepted manifest for `AlignConstant` now points to this shared source, preserving exactness; there is no decrease in verified functions.

`AlignString`'s recovered source additionally uses the ordinary original-style `for (int i; i < alignment - offset; ++i)` and `strcat(space, text + (space - formatted))`. These operations reproduce its entire original **145-byte** length with **143/145 instruction bytes**, original BSS `formatted[256]` object at `0x0808aae0`, and all **eight relocations**. Remaining differences are only offsets 41 and 47: for `strchr`'s space-character parameter the original GCC 3.3.3 chose EDX (`mov edx,0x20` / `mov [esp+4],edx`) while the recovered object chose EAX. This is an unresolved register-allocation issue, **not** an exact match. The strict exact manifest excludes `AlignString`; `vuc/config/align_string_probe.json` is tracked only by the experimental selected manifest.

The experimental probe uses `vuc/probes/align_string_real_unit_oct13.cpp`, a thin include wrapper around `vuc/src/AsmOutputUnit.cpp`. This keeps incomplete functions under `vuc/probes/` as required by the coverage ledger, without duplicating their C++ implementation or changing machine code.

With the authentic `AlignConstant` addition, the combined object has a five-literal `.rodata` section comprising **127/127 verified bytes** at real original ELF addresses, without assigning synthetic section base addresses:

| Content | Compiled offsets | Real original ELF address | Bytes |
| --- | --- | --- | ---: |
| Prologue string | 0..91 | `0x080720a0` | 92 |
| Epilogue string | 92..110 | `0x080721c4` | 19 |
| `AlignConstant`'s `0x` | 111..113 | `0x080721d7` | 3 |
| First constant directive `.assign` | 114..121 | `0x080721f0` | 8 |
| Fallback directive `.equ` | 122..126 | `0x080721fb` | 5 |

`vuc/config/align_constant_exact_probe.json`, `vuc/config/asm_output_tab_constants_probe.json`, and `vuc/config/align_string_probe.json` now all independently verify this **complete**, identical literal section mapping against the original executable. The first two remain exact. This is still a *partial source unit*, because `AsmOutput_PrintInstruction` and `AsmOutput_Emit`, native linker/exception metadata, source object boundaries and some neighboring function bytes remain unresolved.

## Negative controls after studying the new main commit

- `vuc/probes/evaluate_align_string_real_unit_oct13.py` (preserved from the interrupted session) found the genuine 143/145 `AlignString` loop variant. `vuc/probes/evaluate_align_strchr_receiver_oct13.py` then tested 21 ordinary character argument, `int`/`char`/`unsigned char`/receiver-alias variants under three GCC 3.3.3 option sets with full source-unit literal verification; none improved the remaining two register bytes.
- `vuc/probes/evaluate_align_constant_context_oct13.py` tested placing the genuine `AlignConstant` before or after the scanner. In the true source order, GCC 3.3.3 preserves the 143/145 `AlignString`, exact `AlignConstant` and exact `TabConstants`; putting it after the scanner changes the literal section order and fails verified original literal mappings.
- `vuc/probes/evaluate_align_space_register_flags_oct13.py` tested a bounded list of 36 period-plausible GCC optimization, loop/jump alignment, i386 scheduling and constant-merging controls. None produced 145/145. Flags that change the genuine `TabConstants` or original `AlignString` function lengths were rejected; historical 2004 compiler flags are not assumed to match VCL's earlier compiler flags.
- `vuc/probes/evaluate_accumulatorable_switch_order_oct13.py` compared six lexical orders of the three real cases (11, 13, 12) across six real C++ switch/nested/if expression forms with four compiler options (**144 attempts**). The closest original-size code still matches **61/77** in the original 11/13/12 early-return form. The VCL switch-order trick is supported in its own GCC 3.2.2 context but does not alone reproduce VUC's 2004 accumulator predicate under GCC 3.3.3.

These are controlled negative results and preserve legitimate C++ reconstruction. They do not justify hand-written register asm, synthetic alignment fillers, overriding machine code, or accepting partial original functions.

## Acceptance and verification

The exact VUC manifest still contains **188/298 original functions**, **12,702/12,702 accepted instruction bytes**. With `AlignString` classified experimentally, the original 298-function ledger now contains **188 EXACT, 12 EXPERIMENTAL, 98 UNRESOLVED**. The initialized-data and BSS acceptance gates remain independent. No whole-VUC-ELF byte-for-byte identity is claimed.

Reproduction after the rebase:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-operand-rebase-188
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-gate-operand-rebase-188
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_bss_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/bss-gate-operand-rebase-188
```

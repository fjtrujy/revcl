# AsmOutput_TabConstants: 127/127 genuine application method with original AsmOutput source-unit context

Date: 2026-10-13. Branch: `vuc`.

## New byte-exact function

The original `AsmOutput_TabConstants(InstructionList&)`, mangled `_Z22AsmOutput_TabConstantsR15InstructionList`, starts at `0x08049df0` in the original unmodified VUC ELF and comprises exactly **127 machine-instruction bytes**. The accepted source is `vuc/src/AsmOutputUnit.cpp`; it matches **127/127** under the recovered January 2004 GCC 3.3.3 and GNU assembler 2.15 wrapper using `-O2 -frename-registers -falign-jumps=4`.

This is a genuine C++ function scanning each instruction's `ToString()`, comparing the first `.assign` and fallback `.equ` delimiters, measuring the column position, and increasing `constantsTabStop` when needed. Its ELF32 relocation set comprises seven entries: the real `Instruction::ToString()` at `0x08051900`, `strstr` at `0x0804981c` and the original `constantsTabStop` global at `0x0808aac4`. All seven entries resolve exactly.

The original-size bare `vuc/probes/asm_output_tab_constants.cpp` source had reached **118/127** with seven relocations and 13 directly checked literal bytes under `-O2 -falign-jumps=4`. Tests of 21 natural signed/unsigned vector-index, size, pointer/local-lifetime, guard and `strstr` C++ variants were preserved in `vuc/probes/evaluate_tab_constants_source_forms_oct13.py`; no isolated source change improved 118/127.

The decisive evidence came from a **genuine source-unit context**, not unrelated preceding padding or hand-edited instruction bytes. The original order around this region was:

1. `AsmOutput_WritePrologue(std::ofstream&)`, original `0x08049b40`.
2. `AsmOutput_WriteEpilogue(std::ofstream&)`, original `0x08049b60`.
3. `AlignString(char const*, int)`, original `0x08049b80`.
4. `AsmOutput_TabConstants(InstructionList&)`, original `0x08049df0`.

The first two are the actual original output prologue/epilogue C++ string statements, and `AlignString` is a meaningful reconstruction of the 256-byte static text buffer and whitespace alignment. The fourth function is the real instruction constant-column scanner. Combining the two ostream helpers before the scanner in `vuc/probes/evaluate_tab_constants_real_asm_unit_oct13.py` raises the scanner from 118/127 to **125/127**. Inserting the original-neighbor `AlignString` between epilogue and scanner changes GCC 3.3.3 register renaming for the `.equ` fallback literal from EAX to original ECX, producing **127/127**. The 125/127 differences at original offsets 107 and 113 were the source-level compiler-register interference which that unit context resolved.

The accepted source unit's `.rodata` contains **124 verified literal bytes**, all mapped independently to the unchanged original ELF:

| Source literal | Source section offsets | Original ELF address | Verified bytes |
| --- | ---: | --- | ---: |
| Prologue text ending `--endenter` | `0..91` | `0x080720a0` | 92 |
| Epilogue text ending `--endexit` | `92..110` | `0x080721c4` | 19 |
| First constant directive `.assign` | `111..118` | `0x080721f0` | 8 |
| Fallback directive `.equ` | `119..123` | `0x080721fb` | 5 |

No source-unit section bytes are omitted from this verified literal mapping. Recompiling from committed `vuc/src/AsmOutputUnit.cpp` (not a temporary generated file) independently reproduces the **127/127** and seven relocations with the same 124 literal bytes.

**Boundary:** This does not mean all four methods have become byte-exact in the combined object. `AsmOutput_WritePrologue()` remains **22/30** even in this source unit, and `AsmOutput_WriteEpilogue()` also emits 22/30 with this exact scanner configuration; the previously accepted independent 30/30 `AsmOutput_WriteEpilogue` source remains available and separately gated. `AlignString` is still unresolved. Historical whole-linker section ownership, symbol order and native `.eh_frame` layout remain unresolved. We must not mistake the scanner's per-function exactness for linked-ELF identity. The negative-control reproduction and neighboring function results are recorded by `vuc/probes/evaluate_asm_real_unit_neighbors_oct13.py`.

## Other bounded source experiments

The unfinished `Instruction` arena destructor `__tcf_1` remains **45/46 bytes** (the epilogue `pop %eax` versus original `pop %ecx`). `vuc/probes/evaluate_instruction_cleanup_equivalents_oct13.py` tests the real arena allocator source unit with ten ordinary C++ loop/alias/cleanup variations and eight historical compiler configurations; none changes this final byte. `vuc/probes/evaluate_instruction_cleanup_with_formatted_add_oct13.py` also tests genuine `InstructionList` formatted `Add` and `AddB` source-order neighbors, rather than fabricated assembly instructions or prefixes.

`yyFlexLexer::yy_create_buffer(std::istream*, int)` is **131/139 bytes**, with five verified relocations and 44 literal bytes in its original fatal-memory message. `vuc/probes/evaluate_yy_create_real_lexer_unit_oct13.py` tests recovered original lexer methods as genuine translation-unit predecessors/successors; `vuc/probes/evaluate_yycreate_receiver_lifetimes_oct13.py` tests original-style pointer aliases and allocator-sized locals. The exact 139-byte length is reproducible, but eight register-choice differences persist.

`script_error(char const*)` is independently **155/161 bytes**, with 15 relocations and five verified string-literal bytes. Recompiling it beside the real accepted `script_lex` function leaves its six source/argument-register choices unchanged; `vuc/probes/evaluate_script_error_tu.py` retains that evidence. `IsAccumulatorable(IR::Operator_Data*)` is **61/77** with zero relocations under historical GCC 3.3.3 and an authentic preceding `GetOpType` C++ method; `-falign-jumps=1` is needed for the original function length and recorded in its probe config.

These three applications have been promoted **only to the diagnostic/EXPERIMENTAL ledger** (`vuc/config/selected_candidates.json`), *never* to the exact manifest. All of the above probes use ordinary C++, preserve genuine original-era semantics as far as recovered, and never mutate the original ELF, patch emitted bytes, or substitute invented relocation addresses.

## Gate and remaining work

The new strict baseline is **188 EXACT**, **11 EXPERIMENTAL**, **99 UNRESOLVED** out of 298 named original functions, totaling **12,702/12,702 accepted original instruction bytes**. The full original linked binary is not yet reconstructed. The native data/BSS and exception/linker matching gates must remain independent from these per-function scores.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-asm-tabconstants-188
```

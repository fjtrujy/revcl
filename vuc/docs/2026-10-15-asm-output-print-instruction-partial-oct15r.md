# VUC AsmOutput_PrintInstruction: first strict 276/382 partial

Date: 2026-10-15. Canonical VUC 2004.1 Linux ELF32/i386 original
`AsmOutput_PrintInstruction(Instruction*, std::ofstream&)`, at
`0x08049c70`, 382 bytes. This original application function was
UNRESOLVED and had no selected reconstructed C++03 body before this
round. Prior work had only recovered its smaller AsmOutput.cpp
predecessors.

## Recovered source-level behavior

The original ELF reads `Instruction::kind` at +8 and
`Instruction::ToString()`, then independently tests the original
string for `.include`, `.macro`, `.endm`, and `.assign`.
If `.assign` is absent it also checks `.equ`. The actual
directive string literals originate in the same `AsmOutput.cpp`
source unit as the existing exactly reconstructed `AlignString`
and `AsmOutput_TabConstants`.

The genuine formatting paths include:
- raw print for labels and directives;
- a newline before `.endm` output;
- for `.assign` and `.equ`, `AlignString` using original
  `constantsTabStop` BSS variable at `0x0808aac4`;
- `strstr("0x")` in the aligned output, followed by in-place
  conversion to the original assembler `H'` hexadecimal format;
- otherwise use fixed 14-column alignment, two spaces, and newline.

The new source `vuc/probes/AsmOutputPrintInstructionFreshOct15r.cpp`
is ordinary historically plausible C++03 using real `std::ofstream`,
`Instruction`, `AlignString` and `strstr`. No inline assembly,
manufactured constants, instruction encoding or fake link relocation
is used.

### Historically authored Boolean dispatch matters

The initial idiomatic candidate used
`bool isConstant=strstr(text,".assign")!=0` and then
conditionally assigned the result of a second `strstr(".equ")`.
Jan25 and Dec06 GCC 3.3.3 emitted only **379 bytes**, not
the original 382. The original assembly initializes `BL=0`
before the fourth `strstr` and sets it to one after a successful
`.assign` match. The old-style:

```cpp
bool isConstant=false;
if (std::strstr(text,".assign")) isConstant=true;
else if (std::strstr(text,".equ")) isConstant=true;
```

emits **382 bytes**, matching the original function size under
both historical compiler snapshots. It preserves the original
short-circuit semantics instead of normalizing the author's
human-written source.

The bounded historical compiler experiment
`vuc/probes/evaluate_asm_print_directive_boolean_oct15r.py`
tests nine genuine bool/int/char/control-flow variants under
both snapshots. The bool `if/else if` and equivalent
bool `goto` forms emit original-length 382 bytes. The
assign-if form emits 379; the int control-flow versions emit
381 or 398. None justifies changing compiler flags broadly.

## Strict acceptance evidence

Manifest:
`vuc/config/asm_output_print_instruction_partial_oct15r.json`.
Both Jan25 and Dec06 GCC 3.3.3 with GNU assembler 2.15
compile the C++03 source under `-O2 -frename-registers`
to **382 original-length bytes**, with **276/382
original-offset instruction bytes** matching,
**25/25 authentic relocation sites**, and **43/43 source
literal bytes** independently verified against original ELF
`.rodata`.

Original literal addresses are:

| Literal | Original ELF address |
| --- | --- |
| `.include` | `0x080721da` |
| `.macro` | `0x080721e3` |
| `.endm` | `0x080721ea` |
| `.assign` | `0x080721f0` |
| `.equ` | `0x080721fb` |
| `"0x"` | `0x080721d7` |
| `"  "` | `0x080721f8` |
| newline | `0x08073070` |

All eight candidate `.rodata` ranges cover the emitted
43-byte section completely. Relocations are normalized only to
authenticated original named callees and the original
`constantsTabStop` BSS symbol. No residual unowned
literal-padding bytes remain in this standalone candidate.

**276/382 is EXPERIMENTAL, not byte-exact.**
The candidate is selected only in
`vuc/config/selected_candidates.json`; it is not part of
the exact manifest or accepted `vuc/src`. It advances
inventory counts to **204 EXACT / 35 EXPERIMENTAL /
59 UNRESOLVED** across 298 functions.

## Genuine original AsmOutput.cpp translation-unit experiment

Rather than sweeping more flags, the repeatable
`vuc/probes/evaluate_asm_output_real_unit_oct15r.py`
compiles this function with its actual accepted original
`AsmOutputUnit.cpp` predecessors, including the fully
reconstructed `AlignString`, `AlignConstant` and
`AsmOutput_TabConstants` methods, preserving their source
definitions and original class layout. It compares the
original-order appended body with a front-of-TabConstants
source-order control.

Both GCC snapshots still produce **382-byte printer bodies,
25 relocations and 276/382 diagnostic matching code bytes**,
identical to the strict standalone result. This eliminates
those genuine predecessor bodies as sufficient explanation
for the remaining code differences. The combined source-unit
version emits additional predecessor literals, so its
original-offset comparison is diagnostic only; it is **not**
promoted without authenticating the whole combined section.
The selected standalone source is independently fully
strictly verified.

Generated compiler objects and full ledgers are preserved
outside Git in the existing historical Lima guest at:

- `/tmp/vuc-binutils215/asm-output-directive-bool-oct15r/`
- `/tmp/vuc-binutils215/asm-output-real-unit-oct15r/`

Reproduce the accepted experimental score:

```sh
R=/path/to/revcl/vuc
/tmp/vuc-binutils215/g++-jan25-as215 -O2 -frename-registers \
  -c "$R/probes/AsmOutputPrintInstructionFreshOct15r.cpp" \
  -o /tmp/vuc-binutils215/asm-print-check-oct15r.o
python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/asm-print-check-oct15r.o \
  --config "$R/config/asm_output_print_instruction_partial_oct15r.json"
```

Next, reconstruct the original branch-block ordering of the
label/macro/endm/assign/constant formatting paths from the
disassembly, without artificial source-only block padding.
The complete VUC ELF has no established byte-identical build.

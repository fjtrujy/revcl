# VUC 2004.1 — source-owner / alignment controls after prologue match

2026-11-23, experimental historical GCC 3.3.3 i386 builds.
Original ELF/ZIP are unchanged and are not committed.

## Positive outcome

The earlier 22/30 AsmOutput_WritePrologue is now **30/30** exact
with its 92-byte authentic literal and two relocations.
The genuine AlignString function placed before it in the same source
unit remains **145/145** with its eight authentic relocations.
Both the Jan25 and Dec06 historical compiler snapshots agree.

Reproducer: vuc/probes/evaluate_asmoutput_alignstring_order_20261123.py
Accepted owner: vuc/src/AsmOutputPrologueExactOwner.cpp
Acceptance manifest: vuc/config/asm_output_jan25_probe.json

## Negative controls preserved

- vuc/probes/evaluate_asmoutput_real_order_20261123.py tried
  placing the prologue among the original methods from the larger
  AsmOutputUnit owner. The extra literal strings changed .rodata section
  ownership, so the strict matcher rejected these builds; they MUST NOT
  be scored as authenticated partial matches.
- vuc/probes/evaluate_scope_copy_combined_align_20261123.py tested
  128 historical compiler builds spanning jump and label alignment
  values 1..8 under both recovered GCC 3.3.3 snapshots for the 149-byte
  Scope normal-iterator uninitialized-copy function. Generated function
  lengths were 136,137,139,143,147,151 or 159 bytes; **none were 149**.
  Adding artificial padding is not a valid solution.
- vuc/probes/evaluate_instruction_arena_definition_order_20261123.py
  compiled 72 genuine InstructionList / arena definition placements
  (nine insertion positions, two method/data placements, two historical
  compilers, two alignment settings). All authenticated cleanup bodies
  stayed at **45/46**. No recovery of the dead pop-register byte.

All generated CPP/object/result files remain in the Linux guest's
/tmp directories. Existing accepted VUC source code, original data and
BSS inventories remain unchanged; no independent whole-ELF identity
is claimed. The PR proposing integration into main remains closed.

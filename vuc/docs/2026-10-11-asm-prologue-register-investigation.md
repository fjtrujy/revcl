# VUC assembly prologue register scheduling

The original 30-byte `AsmOutput_WritePrologue(std::ofstream&)` at
`0x08049b40` loads the string address into EAX before establishing EBP,
then loads the stream into EDX before storing the two call arguments.

The existing natural C++ source in `vuc/probes/asm_output_hypothesis.cpp`
emits the same length with January 2004 GCC 3.3.3 but loads the literal
into EDX, stores it immediately, and only then loads the stream into
EDX. Its verified score is **22/30**, including both resolved original
relocations and the 92-byte literal. This is not an accepted function.

The following compiler hypotheses were tested against the original ELF
using `vuc/scripts/match_probe.py`:

| Flags | Result |
| --- | --- |
| `-O2` | 22/30 |
| `-O2 -frename-registers` | 22/30 |
| `-O1` | 29-byte function, non-exact |
| `-Os` | 20-byte function, non-exact |
| `-O2 -fno-reorder-blocks` | 22/30 |
| `-O2 -fno-schedule-insns2` | 14/30 |

The differences are consistent with instruction scheduling and register
lifetime decisions, not a missing literal or relocation. Investigate
the original `AsmOutput.cpp` translation-unit context and historical
compiler build before promoting this function. No assembly byte
fabrication or manual instruction encoding is permitted.

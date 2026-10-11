# Cross-function negative reconstruction evidence

These results are reproducible investigations, not accepted functions.
The original VUC ELF remains the sole authority for code bytes and
relocation targets. All compiled variants use the recovered January
2004 GCC 3.3.3 and GNU as 2.15 toolchain in `revcl-oracle`.

## Different functions investigated

| Function or group | Probe | Best result | Conclusion |
| --- | --- | --- | --- |
| `Print(ASTFunCall*)` | `evaluate_print_fun_call.py` | 96/98 | 11 source shapes, four flag sets; no exact match |
| `Printer_UglyPrint()` | `evaluate_ugly_print.py` | 38/40 | Nine source shapes and nine flag sets; EAX/ECX register mismatch |
| `Canonize2_Reduce()` | `evaluate_canonize2.py` | 83/85 | 13 source shapes, two flag sets; EAX/EDX register mismatch |
| Two `AsmVariable` STL copy helpers | `evaluate_asmvariable_copy.py` | 67 and 74 matching bytes | 14 copy representations, three flag sets; no exact match |
| `Value::operator+` | `evaluate_value_plus.py` | 172/219 | Switch-case reorderings and explicit default under jump alignment 3 or 4; no exact match |

The `Value::operator+` source-order experiment is especially useful:
six variants produce the same 172/219 result with the selected jump
alignment. The `if/else` and independent-condition alternatives have
different function lengths and cannot be accepted. For the STL copy
helpers, aggregate/pair-based copies perform better than the original
two-field explicit copy hypothesis, but neither helper is exact.

## Avoid repeating these searches

Changing only equivalent boolean spelling, temporary references,
source-local aliases, switch case order, or routine optimization flags
has not recovered the missing register choices. Next batches should
prefer distinct unresolved functions and authentic source/ABI evidence,
then return to near-exact functions only with a new compiler or source
layout hypothesis. Do not promote partial matches or fabricate code.

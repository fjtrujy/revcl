# Instruction cleanup and word scanner reconstruction probes

Both targets were tested against the original VUC ELF with the recovered
GCC 3.3.3 / GNU as 2.15 toolchain. These results are not accepted.

## Instruction arena cleanup

`evaluate_instruction_cleanup.py` tested ordinary C++ loop and pointer
representations. The best candidate consistently matched **45/46**
bytes; none matched all 46. `evaluate_instruction_cleanup_flags.py`
also tested 21 compiler options, with no improvement beyond 45/46.
Some options change function length and cannot be accepted.

## GetNextWord

`evaluate_next_word_shapes.py` tested six character-class guard
representations with two compiler configurations. All twelve generated
functions had a different length from the original **135-byte**
function. No byte-exact candidate was obtained.

These failures narrow the search space. A new attempt at the 45/46
cleanup function should begin with a precise instruction-level diff
and an original-source hypothesis, not more equivalent loop spellings.

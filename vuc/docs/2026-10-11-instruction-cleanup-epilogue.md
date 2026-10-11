# Instruction arena cleanup: isolated epilogue mismatch

The original `__tcf_1` at `0x08051980` has 46 bytes. The historical
GCC 3.3.3 (20040125) + GNU as 2.15 reconstruction has **45/46**
matching bytes, with the same instruction lengths, control flow,
relocation-resolved addresses and calls.

The only differing byte is the register chosen to discard the local
stack slot immediately before restoring EBX and EBP:

```asm
original:  080519aa: 59    pop %ecx
candidate:       aa: 58    pop %eax
```

`vuc/probes/evaluate_instruction_cleanup_epilogue.py` reproduces a
21-configuration optimizer/ABI experiment. None of the tested flags
produces an exact match. Most retain 45/46; several change the
function length and are therefore disqualified. No hand-written
assembly or patched machine bytes were used.

Next: identify a source-level difference that changes GCC's dead
register preference in the epilogue, or move to another unresolved
function. The candidate must still pass strict whole-function byte
and relocation checks before entering the accepted manifest.

# VUC constant tab-width scan: original source hypothesis

Date: 2026-10-11. Branch: `vuc`.

## Recovered behavior and ELF evidence

The original 127-byte `AsmOutput_TabConstants(InstructionList&)` at
`0x08049df0` traverses the `InstructionList` vector of instruction
pointers, obtains each `Instruction::ToString()`, and searches for
the `.assign` directive, falling back to `.equ`. It updates the
global `constantsTabStop` to the greatest number of characters
preceding a found directive.

The source reconstruction is kept as an **unaccepted** candidate in
`vuc/probes/asm_output_tab_constants.cpp`; its matcher configuration
is `vuc/config/asm_output_tab_constants_probe.json`. The original
ELF provides seven relocations: one `Instruction::ToString()` call,
two `strstr()` calls, two updates/references to `constantsTabStop`,
and two string-literal addresses.

The two original strings are not adjacent in the linked ELF. The
original `.rodata` has `.assign\0` at `0x080721f0`, an unrelated
two-space literal, then `.equ\0` at `0x080721fb`. A standalone
C++ probe legitimately merges its two own strings into a contiguous
13-byte `.rodata` section. The relocation-aware matcher resolves
each string through a separately authenticated literal range:
8 bytes at `0x080721f0` and 5 bytes at `0x080721fb`. No artificial
padding, copied instruction bytes, or original-address constants
are added to the source.

## Historical compiler tests

Using GCC 3.3.3 (20040125 prerelease) and GNU as 2.15:

| Candidate flags | Emitted size | Matching bytes | Status |
| --- | ---: | ---: | --- |
| `-O2 -frename-registers` | 135 | Not comparable | Non-exact |
| `-O2 -frename-registers -falign-jumps=4` | 127 | 112/127 | Non-exact |
| `-O2 -falign-jumps=4` | 127 | **118/127** | Non-exact |

The 118-byte candidate resolves all seven original ELF relocations
and verifies all 13 literal bytes. The nine remaining differences
are register-encoding bytes at offsets 12, 15, 18, 33, 36, 87,
90, 107 and 113 (zero-based). The original uses EDX for the
initial vector pointer, ECX for the loop's later vector-base reload,
and ECX for the second literal. The closest candidate chooses EAX
for the initial pointer and second literal, and EDX for the reload.
Adding `-frename-registers` fixes some of these selections but
changes others, so neither compiler hypothesis is byte-identical.

`vuc/probes/evaluate_tab_constants.py` tests 15 ordinary C++ loop,
pointer, expression, and local-variable forms with 17 historical
optimization and alignment hypotheses. The expanded sweep did not
produce an exact function: the best remains **118/127**. All generated objects are kept
inside the existing Linux VM under
`/tmp/vuc-binutils215/tab-constants-variants/`.

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_tab_constants.py
```

This is a source/ABI hypothesis, not an accepted exact function or
authenticated original `AsmOutput.cpp` translation unit. No source
has been added to `vuc/src/` or to the exact manifest. The ultimate
acceptance gate still requires the **entire original VUC ELF** to be
identical byte for byte.

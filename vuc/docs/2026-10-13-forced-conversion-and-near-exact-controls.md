# VUC: typed arena controls and forced Value conversion

Date: 2026-10-13. Branch: `vuc`. Baseline commit: `05da0eb`.

This work uses the historical GCC 3.3.3 December 6, 2003 and January
25, 2004 prerelease compilers with GNU as 2.15 in `revcl-oracle`.
The original, ZIP-backed executable is inspected read-only. Neither
manually written assembly, register constraints, injected padding,
original binary bytes, nor invented relocation addresses are used.

## Newly reconstructed partial application function

The original `Value::TypeConvertForce(TypeID, Value const&, Value
const&, Value&, Value&)` starts at **`0x0806f8b0`** and has **257
instruction bytes**. The source-level reconstruction now matches
**193/257 original bytes** (zero relocations or initialized literals),
compiled with:

```
-O2 -falign-jumps=1 -falign-labels=1 -falign-loops=2
```

- Candidate source: `vuc/probes/ValueTypeConvertForceUnit.cpp`
- Experimental manifest: `vuc/config/value_type_convert_force_oct13_probe.json`
- Original-ELF and historical compiler reference: existing
  `vuc/scripts/match_probe.py`.

The reconstructed function performs the original 32-byte Value copies,
returns immediately when both inputs already have the requested type,
otherwise checks whether the requested type permits conversion, and
uses normal C++ switch cases to convert type-3 float payloads to integer
with GCC's x87 `fistp` and FPU control-word save/restore. The preserved
type tags and representation details are inferred from the original
instruction stream and remain provisional at the source semantic level.

The original RTL schedule and register choice are not fully recovered.
GCC selects EDI, ECX, and EDX in different order at offsets 16-25.
Some remaining mismatches affect the return-block layout and the x87
conversion code. **193/257 is partial, not byte-exact.**

### Real predecessor source and positive controls

The original `Value::TypeConvert()` definition precedes this function
in the ELF, and is already independently verified **220/220** under
its accepted historical `-frename-registers` settings.
`ValueTypeConvertForceUnit.cpp` includes this exact-source C++ method
before the forced-conversion body. In that larger, genuine sequence,
using the **forced-conversion candidate's different plain `-O2`
flags**, TypeConvert itself becomes 212/220, so the combined object
**must not be claimed as an exact source unit**.

- `evaluate_type_convert_force_control_oct13.py` tests nine
  semantically equivalent early-exit and dispatch shapes with ordinary
  switch/if case forms and GCC flags. Reversing lexical case 2/3 order
  produces a candidate with the original 257-byte size and **192/257**
  matching code in isolation, whereas the initial shape emitted 273
  bytes with the normal historical renaming option.
- `evaluate_type_convert_force_context_oct13.py` tests 108 genuine
  contextual compiler/label/loop alignment cases. Adding the
  independent authentic preceding TypeConvert definition improves
  the best 257-byte match by one more byte, **192 -> 193**.
- `evaluate_type_convert_force_flags_oct13.py` tests 27 period
  optimizer choices with both historical compiler dates and with/without
  register renaming. The best remains **193/257**; no option generates
  a new fully exact function. More alignment or compiler-date changes
  are not supported by this evidence.

A strict new manifest and regression test keep the reconstruction in
`vuc/probes/` and **out of** `vuc/config/exact_candidates.json`.
The exact accepted TypeConvert source and manifest remain unchanged.

## Two closest near-exact functions remain unresolved

### AlignString at `0x08049b80` (143/145)

`evaluate_near_exact_compiler_dates_oct13.py` compiles the genuine
`AsmOutputUnit.cpp` source through both recovered GCC 3.3.3 compiler
dates and five period optimization/control combinations, authenticating
its original BSS, five original literal ranges and PLT relocations.
Both historical compiler dates produce **143/145** with the exact
145-byte length and the same differences at byte offsets 41/47:
EAX rather than the original EDX for `strchr`'s space character.
No compiler-date change or tested flag restores the original pair.
The exact `AlignConstant` and `AsmOutput_TabConstants` sources must
remain protected by their independent accepted checks.

### Instruction arena destructor at `0x08051980` (45/46)

The same compiler-date matrix reproduces the original-length
**45/46** result; the final epilogue stack discard remains `pop %eax`
rather than the original `pop %ecx`.

`evaluate_instruction_typed_arena_oct13.py` goes beyond C++ loop
rewrites by testing **four authentic typed arena-chunk record layouts**
whose next pointer is at byte `0x2004`, with five natural while/for/
do/delete lifetime forms and three ordinary optimizer configurations.
These objects include a genuine static constructor/destructor, iostream
startup, and the original `Instruction::instrData` static field:
all valid original-sized candidates remain **45/46**. This rules out
the tested typed-pointer record explanations without presuming the
original declaration spelling.

## Reproduction and remaining constraints

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_type_convert_force_control_oct13.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_type_convert_force_context_oct13.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_type_convert_force_flags_oct13.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_instruction_typed_arena_oct13.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_near_exact_compiler_dates_oct13.py"
```

The benchmark record is now **190 EXACT, 15 EXPERIMENTAL, 93
UNRESOLVED** among the original 298 named functions. The 190 accepted
functions still cover **13,144/13,144 checked instruction bytes**. All
244 original initialized-data objects and 12 BSS declarations remain
covered by independent strict checks.

Do not promote the forced-conversion partial, the AlignString near
match, or the arena destructor to exact acceptance. The complete linked
VUC ELF has **not** been demonstrated byte-identical; reconstituting
genuine source-unit ownership and the historical linker remains open.

# Enum-typed storage, string domains, and remaining compiler flags

All accepted changes must reproduce the original stripped VCL SHA-256 using the
Red Hat 9 GCC 3.2.2/i686 toolchain:

`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`

## Actual enum-typed fields

C++98 enum values alone do not type a field. REVCL now also uses a GCC
machine-mode enum alias for byte-sized storage originally represented by
`char` or `unsigned char`:

`typedef VclCodeMode VclCodeModeByte __attribute__((mode(QI)));`

Both GCC 3.2.2 and Apple Clang accept this syntax. The enum type remains
one byte; C++98 compile-time size assertions in `src/vcl_modes.h` catch
widening. A regular four-byte enum or modern `enum class` would alter
the historical ABI.

| Owning type | Field | New enum type | Original storage |
| --- | --- | --- | --- |
| `InstructionStream` | `startType` | `VclStreamStartByte` | `unsigned char` |
| `InstructionStream` | `endType` | `VclStreamEndByte` | `unsigned char` |
| `ScheduleTable` | `codeModeValue` | `VclCodeModeByte` | `unsigned char` |
| `ScheduleTable` | `rawSchedulingMode` | `VclRawSchedulingModeByte` | `unsigned char` |
| `ScheduleTable` | `allocationMode` | `VclAllocationModeByte` | `char` |
| `Operand` | `direction` | `VclOperandDirectionByte` | `char` |
| `Operand` | `operandKind` | `VclOperandKindByte` | `char` |
| `SourceLocationEntry` | `locationKind` | `VclSourceLocationKind` | `int` |

The two originally signed `Operand` fields require a negative enum
enumerator to preserve GCC's signed one-byte representation. These
negative markers enforce the storage domain, **not** a known runtime
semantic value. Without them, the ELF hash changed even though the
field size stayed one byte.

Typed getter signatures were also recovered for
`InstructionStream::getEndType`, `getCodeMode`,
`getRawSchedulingMode`, `getAllocMode` and
`ScheduleTable::codeMode`, all passing the exact-SHA test.
`Operand::componentMask` still uses `unsigned char`: an attempted
byte-enum conversion changed the historical machine code and was
reverted. Layout checks alone are insufficient proof of byte identity.

## String labels and enums

**A C++ enum cannot hold string values.** The appropriate pattern is an
enum key or category plus the existing string-producing switch or
string table. Centralizing diagnostic literals in new global arrays
can change their addresses and the original ELF data layout.

The project already has enum-keyed mappings for operand kinds, code
modes, raw scheduling/allocation modes, stream boundaries and stall
reasons. This batch expands these patterns:

- `VclStallType` selects the existing `LATENCY`, `THRUPUT` and
  `RFWRITE` strings via `stallTypeNames` and defines the
  shared `VCL_STALL_TYPE_COUNT` for loops and array size.
- `VclSourceLocationKind` identifies `NONE`, `FILE` and `MACRO`
  while `SourceLocationEntry` still returns the original
  `"NULL "`, `"FILE "` and `"MACRO"` diagnostic labels.
- The integer enum values already introduced in the prior PR continue
  to select the original 55 operand-kind names and other textual names.

These relationships remove numeric-to-string coupling without
reordering or rewriting source literals, which is necessary for the
byte-exact ELF. Arbitrary error messages and hardware mnemonics
should not become one giant string enum.

## Historical compiler flag investigation

The preceding PR eliminated ten of 20 per-translation-unit exceptions.
The remaining five uses of `-fmerge-all-constants` can now be moved
to the **common** GCC C++ options. A complete 131-source rebuild with
global `-fmerge-all-constants` still reproduces the original SHA.

The other five per-TU exceptions have specific, experimentally observed
effects:

| Source file | Flag | Difference without the flag |
| --- | --- | --- |
| `RegWeb.cpp` | `-fno-weak` | Deferred-inline pragma setup omits most RegWeb method definitions |
| `wrangle.cpp` | `-fno-cse-follow-jumps` | GCC drops a 4-byte push of `table->loopSize` in the mode-1 worker |
| `wrangleCore.cpp` | `-fno-cse-follow-jumps` | Same argument-push difference in the mode-0 worker |
| `doPostOpt.cpp` | `-mno-fp-ret-in-387` | Float-return declaration reads an actual integer EAX return |
| `infoList.cpp` | `-finline-functions` | Derived destructors stop inlining the empty base destructor |

The previous five `-fmerge-all-constants` overrides were associated
with **externally referenced diagnostic arrays** placed in
`.rodata.str1.32` versus `.rodata`. In those files, the compiler
produced identical `.text` sizes without the flag; the mismatch was
in literal section placement. Making the flag global is both simpler
and independently verified to preserve the complete executable.

### Source alternatives tested and reverted

- Corrected `doPostOpt`'s conditional float-vs-int return signature
  and tried direct storage, integer/union locals, temporary variables,
  and helper wrappers. GCC changed stack slots or the relative order
  of the `movl %eax` and `addl $0x10, %esp` instructions.
  **None matched** without its per-TU option. The header and original
  source workaround were restored.
- Tried ordinary inline/out-of-line base destructor definitions and
  a GNU `always_inline` annotation for `infoList`. These introduced
  weak symbols or retained calls to the base destructor, changing
  both bytes and addresses. Reverted.
- Replaced RegWeb's deferred-inline pragmas with ordinary out-of-line
  definitions and tried removing the pragmas. These generated
  different function bodies or linker errors. Reverted.
- Tried making each of `-mno-fp-ret-in-387`, `-finline-functions`,
  `-fno-cse-follow-jumps` and `-fno-weak` a project-wide flag
  alongside `-fmerge-all-constants`. **All four trials** changed
  the final stripped SHA.

Therefore `config/build.json` now contains three common flags
(`-O2`, `-frename-registers`, `-fmerge-all-constants`) and only
**five** exceptional source files. The standard RH9 linker script
and explicit 131-object link input ordering are retained.

**None of these five per-unit options is proven to have been present
in Sony's original build.** They are requirements of our current
reconstructed source shape, not necessarily of the original program.
Future matching work should target their identified causes and compare
instruction sequences, symbol placement and final SHA, rather than
merely testing functional equivalence.

## Verification

Run the modern macOS/Clang build and 13 native tests, the historical
GCC 3.2.2/i686 build and exact-SHA CTest, plus naming-progress and
`git diff --check` before accepting a new source variation.

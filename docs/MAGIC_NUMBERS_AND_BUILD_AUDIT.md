# Magic numbers and historical build simplification audit

This audit is intentionally separate from the completed identifier-naming queue:
there are still numeric literals in REVCL. Only constants whose **domain and
values are recoverable from actual consumers** have been replaced. Every source
change is conditional on reproducing the exact historical stripped ELF.

## Recovered numeric domains

| Domain | Source of evidence | Representation |
| --- | --- | --- |
| VCL table code modes (0–6) | `codeModeStr`, table initialization and mode transitions | `VclCodeMode` enum |
| Operand direction (0–3) | input/output tests and bidirectional register use | `VclOperandDirection` enum |
| Stream start/end kinds | `istStartTypeName` and `istEndTypeName` | `VclStreamStartKind` / `VclStreamEndKind` enums |
| Raw scheduling modes (0–4) | `rawModeName` and free/fixed-mode checks | `VclRawSchedulingMode` enum |
| Allocation modes (0–3) | `allocModeName` and allocation lifecycle | `VclAllocationMode` enum |
| XYZW component bits | validation and component-letter decoding | `VclComponentMask` enum |
| Operand kinds (0–55, with unused index 12) | `opKindName`, decoding, register-class selection and STAT expansion | `VclOperandKind` enum |
| Stall reasons (0–10) | `stallReasonStr` and recorded reason assignments | `VclStallReason` enum |
| Fixed storage capacities | array declarations and explicit limit guards | `VclFixedCapacity` integer enum |
| STAT register indices 55–58 | `regDescTable.h` and `expandVirtualStatOperand` | function-local enum |
| STAT immediate masks | `expandVirtualStatOperand` dependency and mask tests | function-local enum |

These are **integer enumerators**, not C++ enum-typed struct fields. All
historical `char`, `unsigned char`, `short` and `int` storage and parameter
types are unchanged, including ABI layout. There are no macro replacements
for function names and no new runtime variables. Source and error string
literals remain unchanged.

The operand-kind values 18–21 share the generic `OP_V_STAT` text in
`opKindName`. Their more specific names come from the distinct ALU/DIV and
shadow register allocations in `Instr::expandVirtualStatOperand`, not from
the diagnostic label alone.

**Not yet proven:** literal opcode IDs, packed lock constraint/return masks,
some architecture-specific register classes, specialized bitfield encodings,
the meaning of every sentinel value (`-1`, `30000`, etc.), or arbitrary
constants used only in isolated assembler-reconstruction logic. Similar
numeric values in different domains must not be given the same name without
control-flow and data-flow evidence.

## Historical GCC 3.2.2 build experiments

All experiments used the same pinned Red Hat 9/i686 container and the
reference SHA-256 of the **stripped** executable:

`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`

For the per-source-flag trials, the alternative source was compiled using
the baseline `-O2 -frename-registers` options *without that translation
unit's additional option*. The resulting object replaced **only the one
object at its original position** in the full 131-object link. That linked
executable was then stripped and checked against the reference. This
isolates the effect of each override without confounding source order.
Some unstripped objects changed but the stripped final executable did not;
**object-byte inequality alone is not a reason to retain an override**.

### Per-translation-unit flags

The original reconstructed configuration had **20 overrides**. Exactly
**10** were individually unnecessary and also survived a combined test.

| Result | Translation units | Additional option |
| --- | --- | --- |
| Retain | `RegWeb.cpp` | `-fno-weak` |
| Retain | `wrangle.cpp`, `wrangleCore.cpp` | `-fno-cse-follow-jumps` |
| Retain | `doPostOpt.cpp` | `-mno-fp-ret-in-387` |
| Retain | `infoList.cpp` | `-finline-functions` |
| Retain | `workTable.cpp`, `explodeRaw.cpp`, `prepExtInfo.cpp`, `hwRegPool.cpp`, `prepReduce.cpp` | `-fmerge-all-constants` |
| Remove | `checkOpt.cpp`, `loadStalls.cpp`, `makeForceTable.cpp`, `regRefLiveRange.cpp`, `Parser.cpp`, `fileEmit.cpp`, `getNumOptimisers.cpp`, `immFixup.cpp`, `makeMove.cpp`, `tidy.cpp` | `-fmerge-all-constants` |

Replacing **all ten removable objects together** reproduces the reference
hash, even without the custom linker script. `config/build.json` therefore
keeps only ten proven-required exceptions. Their necessity is **for the
current reconstructed sources and exact hash**, not evidence that Sony's
original build used these special options. Split/merged translation units,
compiler heuristics and decompilation expression choices may eventually
make additional overrides unnecessary.

The JSON was also simplified: `cxxflags` now declares the common options
once and `units` maps each exceptional source file directly to **only its
extra flags**. The redundant copy of the common flags in every entry and
the unused `cflags` section (the project only builds C++ sources) were
removed. This is a representation change, not a compiler-behavior change.

The common `-frename-registers` flag remains. Removing it from **main.cpp**,
**Instr.cpp** or **wrangle.cpp** individually changed the stripped ELF.
This proves the flag matters for multiple units, though it does not prove
every one of the 131 files needs it.

### Object link order and source filenames

The 131 objects were linked using alternative orders **without
recompiling source code**:

| Link experiment | Reference SHA matched? |
| --- | --- |
| Existing recovered object list | **Yes** |
| Swap adjacent `preprocessorTools.cpp` and `os.cpp` objects | **No** |
| Alphabetically sort all 131 objects | **No** |
| Copy an object under a different filename but keep its input position | **Yes** |

This distinguishes three independent things:

1. **Compilation sequence** is not fixed. The files can compile in
   parallel; the resulting object input ordering is what the linker uses.
2. **Object filenames** do not establish order. Names are readable
   identifiers of units, not sorting directives for the linker.
3. **The `VCL_SOURCES` order** currently preserves the recovered code/data
   layout that matches the reference ELF. A traditional Makefile can
   also specify an ordered object list; using an explicit order is not
   uniquely a reverse-engineering workaround.

Renaming all files with numbered prefixes merely to exploit alphabetical
sorting would reduce readability and still require an artificial ordering
mechanism. `config/vcl-link-order.cmake` remains the clearest reproducible
statement of the observed linker input sequence. It does **not** require
building the files sequentially.

### Custom linker script

The formerly supplied `tools/rh9/vcl.ld` is **not required**. Linking the
original object set with the pinned toolchain's default linker script
produced the exact reference stripped ELF; repeating this with all ten
unneeded per-unit overrides removed also passed. The redundant custom
script and the `-Wl,-T` option were therefore removed.

A full standard historical CMake rebuild (not just the object-substitution
experiments) and `exact-sha256` CTest confirm the simplified configuration.
Modern native fixture tests also remain green.

## Further investigations

- Recover opcode IDs from their verified descriptor-table indices, **before**
  substituting numeric instruction opcodes across unrelated contexts.
- Analyze scheduling flag bits, packed error/constraint return codes and
  sentinel bounds before naming them; they can interact with GCC register
  allocation and control flow even when the literal value is unchanged.
- Investigate whether reconstructed translation-unit boundaries explain
  the ten remaining flag exceptions. A successful refactor would need
  source-level evidence and a fresh historical SHA check, not merely a
  functional test.

**Follow-up:** [Enum types and remaining compiler flags](ENUM_TYPES_AND_COMPILER_FLAGS.md)
documents the subsequent successful promotion of `-fmerge-all-constants` to
a project-wide flag, reducing the ten exceptions recorded above to **five**.
It also records exact-SHA-tested enum-typed fields and source-level trials
that did not yet remove the other flags.

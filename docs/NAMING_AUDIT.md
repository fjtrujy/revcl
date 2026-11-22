# Naming audit: VCL and VUC reconstruction

Status: iterative semantic naming pass, 2026-11-22.

This work inventories opaque identifiers and improves them **without changing**
the verified VCL executable. It is deliberately separate from functional
reverse-engineering: a plausible name does not prove a function or field's
meaning, and a name-only change must not change instructions, symbols, object
order or output bytes.

## Reproducible token inventory (C, no Python dependency)

```sh
cc -std=c99 -Wall -Wextra -Werror -O2 \
  tools/naming-audit.c -o /tmp/revcl-naming-audit
/tmp/revcl-naming-audit src > docs/naming-identifiers.tsv
```

The C scanner reads source tokens while ignoring comments and string/character
literals. It identifies *candidates* for renaming, not every unhelpful name:
address-based `func_XXXXXXXX`, `g_XXXXXXXX`, `clsXXXXXXXX`, offset-based
`unk_XX`, numeric temporaries, and address-based source filenames. It does not
try to infer C++ member ownership or judge whether abbreviations like `i`,
`n`, `wt`, or `op` are good within a particular function.

| VCL candidate category | Distinct spellings/files | Notes |
| --- | ---: | --- |
| Address-named functions | 436 | Includes original ABI spellings intentionally preserved at definitions; 10 diagnostic helpers have readable call aliases |
| Offset-named fields | 237 | Identical offsets recur in many unrelated classes |
| Address-coded globals/other identifiers | 85 | Recover names from initialization and cross-references before modifying |
| Numeric class names | 4 | Three have semantic typedefs in the first pass |
| Address-named source files | 0 | All original address-coded translation-unit filenames now have semantic names |
| Explicit numeric placeholder variables | 4 | Not an exhaustive count of ambiguous variables |

**Full, individually locatable token list:**
[`naming-identifiers.tsv`](naming-identifiers.tsv) (identifier, token count,
file count and first source location). Files are sorted by category/frequency.

## Compiler-AST declaration audit

A separate read-only Clang AST examination of all **131 VCL translation units**
found:

| Source declaration candidates | Count | Meaning |
| --- | ---: | --- |
| Owner-qualified `unk_` field declarations | 568 | Pairs of owning type and offset-style field name; the same `unk_04` in two classes counts twice |
| Short/anonymous parameter declarations | 869 | Names with at most two letters, or `arg`/`val`/`ptr`/`tmp`; includes legitimate conventional names |
| Short local variable declarations | 2,248 | Same naming heuristic; includes loop indices that may already be appropriate |

Full declaration review queues are in
[`naming-fields.tsv`](naming-fields.tsv) and
[`naming-declarations.tsv`](naming-declarations.tsv). The latter includes
kind (`parameter_review` / `local_review`), source location, enclosing
function, and Clang-reported type. These are **review queues, not 3,117
confirmed naming bugs**. The initial AST pass compiled the source using the
native CMake `compile_commands.json` flags. Each function and variable is
listed by the source location of its definition rather than by duplicated
header inclusions. The queues have been adjusted for the accepted renames;
rerun the AST extraction before using them as an exact declaration census.

## VUC branch

The same lexical inventory was run against the 101 committed `vuc/src` source
units from the `vuc` branch. There were no names matching VCL's address/offset
patterns, but the scanner flagged `zzzzMakeRTTIWork`, a provisional virtual
method name. See
[`naming-vuc-identifiers.tsv`](naming-vuc-identifiers.tsv).

This **does not mean VUC is fully named**. The reconstruction contains
experimental types, scaffolding, single-letter arguments and provisional
object ownership. Those require an independent Clang/ABI-based pass against
the active, partially reconstructed VUC sources; a naive rename of virtual
methods or ABI-facing types may affect byte matching.

## First accepted rename batch

- `src/f0804ac44.cpp` -> `src/diagnostics.cpp` (logging and fatal helpers).
- `src/f0804a608.cpp` -> `src/preprocessorTools.cpp` (gcc/gasp driver).
- `CpuDef::unk_04` -> `instructionsPerRawLine`.
- `PoolDef::unk_10` -> `virtualRegisterParts`.
- Four recovered `RegDesc` fields -> `allocatable`, `registerClass`,
  `partCount`, `nonBlockId` (`unk_14` deliberately remains unresolved).
- `RegPool::Init` argument names -> `registerPrefix`, `virtualRegisterParts`.
- 2,915 diagnostic calls / declarations now use semantic source names including
  `reportError`, `reportInfo`, `reportWarning`, `reportMessage`,
  `reportUserMessage`, `formatIntoBuffer`, `parseInteger`,
  `exitAfterCleanup`, `exitWithError`, and `reportVerboseInfo`.
- Three opaque C++ type spellings have descriptive typedefs:
  `InstructionPosition`, `OperandLock`, `RegisterPartLockList`. C++ source uses
  these instead of 201 occurrences of the address-coded type names.
- `src/f080724f0.cpp` -> `src/tableRowAccess.cpp` and
  `src/f080843a0.cpp` -> `src/flowBlockQueries.cpp`.
- Semantic aliases `abortAfterError`, `asciiCaseEqual`, `streamName`,
  and `registerName` preserve their original historical linker symbols.

The helper aliases and class typedefs are **not ABI renames**: the historical
C++ mangled symbols remain as originally linked. Renaming those definitions
would change the reference executable's symbol/relocation tables. The aliases
allow readable source without breaking the byte-exact reproduction.

## Second accepted rename batch: parser and source units

- `Parser` fields: `locationTagCount`, `pendingIncludeName`,
  `pendingMacroName`, `gaspLocationStack`, `gaspLocationDepth`,
  `rawBuffer`, `processedBuffer`, `parsedLineCount`, `parsedLines`.
  The names follow their allocation, lifetime, and use in `Parser.cpp`.
- `LocTag` fields: `tokenCount`, `tokens`; `TokenEntry` fields:
  `textOffset`, `separator`, `inComment`. These follow the tokenizer's
  writes and token lookup; the structure layout and field types are unchanged.
- Ten parser/line-entry methods now have descriptive **source-level aliases**:
  `initializeParser`, `buildProcessedLines`, `getParsedLineCount`,
  `getParsedLine`, `addLocationTag`, `decodeGaspLine`,
  `isAssignmentLine`, `tokenizeSourceLine`, `tokenTextAt`,
  `dumpSourceLine`. Their original mangled symbols are preserved.
- `src/f08085aa8.cpp` -> `src/splitParserStream.cpp`: the unit turns flow
  blocks into streams and binds dependencies.
- `src/cls08058718.cpp` -> `src/instructionPosition.cpp`: it implements
  the already identified `InstructionPosition` type and range helpers.

The revised parser code passed all 13 macOS CTest checks and the historical
`exact-sha256` check. The historical check requires the verified RH9 **i686**
glibc image. The workstation's older default `revcl-rh9` image contains a
different glibc build that changes only imported dynamic-symbol sizes; use
`REVCL_RH9_IMAGE=revcl-rh9-i686-test` locally until the default image tag is
rebuilt from the pinned RPMs.

## Owner-qualified field renaming: Clang AST proof

A Clang AST pass resolved **728 exact source identifier locations in 84 files**
for **10 high-confidence fields**. Unlike a textual rename of `unk_38`, the
pass uses `FieldDecl` owner identities and `MemberExpr.referencedMemberDecl`
IDs to avoid changing unrelated types with the same field offsets:

- `Instr`: `opcodeInfo`, `operands`, `operandCount`.
- `Program`: `streamHead`, `streamTail`, `streamCount`.
- `Processor`: `uniqueName`, `instructionsPerRawLine`.
- `RegPoolEntry`: `registerClass`, `partCount`.

Unqualified inline member uses in `RegPool.h` were checked and updated
separately because their AST node kind differs from `MemberExpr`. No field
layout order, alignment, data types, or original function symbols changed.
**Native 13/13 CTest and the exact original historical SHA-256 still pass.**

## Second AST owner-qualified field batch

Twenty-two more high-confidence fields were renamed at **513 exact AST
source locations across 65 files**, with no class layout or logic changes:

- `RegPoolEntry`: `ownerPool`, `isVirtual`, `allocatable`,
  `assignedPhysicalRegister`, `registerIndex`, `usedSlotMask`,
  `throughputWriter`, `registerWeb`.
- `RegPool`: `registerEntries`, `entryCount`, `zeroRegister`,
  `externalRegister`, `virtualPartCount`, `poolName`.
- `Program`: `uniqueNamePrefixes`, `flowIdCount`, `streamsById`,
  `backTraceVisitCache`.
- `instStream`: `owningProgram`, `nextStream`, `previousStream`,
  `workTablePtr`.

The AST tool's `MemberExpr` matching does not cover unqualified accesses in
inline methods. The nine affected `instStream` inline accesses were patched
separately after a compiler check, then revalidated. **13/13 native CTests
and the original byte-exact historical SHA-256 both passed.**

## Final source-file naming pass

The ten remaining address-coded `.cpp` filenames have been renamed while
retaining **exactly the same translation-unit ordering**:

| Original file | Descriptive name | Evidence |
| --- | --- | --- |
| `f08052f84.cpp` | `instructionBranchFixups.cpp` | Instruction fixup and branch rewrite methods |
| `f080532d8.cpp` | `instructionOperandNormalization.cpp` | Operand order and zero-register normalization |
| `f0806ae58.cpp` | `movePropagation.cpp` | Move-propagation optimizer |
| `f0807aee4.cpp` | `externalStreamPreparation.cpp` | External stream preparation and NOP cleanup |
| `f0807f2c4.cpp` | `localMoveResolution.cpp` | Local move-resolution helper |
| `f0807f308.cpp` | `moveListConsistency.cpp` | Block move-list consistency checks |
| `f0808061c.cpp` | `wrangleCore.cpp` | Core wrangling logic; original `-fno-cse-follow-jumps` flag preserved |
| `f080817d4.cpp` | `wrangleCandidateSelection.cpp` | Candidate selection for wrangling |
| `f08085a9c.cpp` | `flowBuildHooks.cpp` | Flow-build hook and forward dependencies |
| `f0808750c.cpp` | `instructionTextEmission.cpp` | Instruction-output formatting |

For `wrangleCore.cpp`, the special per-unit compiler flags were migrated in
`config/build.json`. The complete original link order in
`config/vcl-link-order.cmake` was preserved. **The historical executable remains
byte-identical** after this last file-renaming pass.

## Rules for future iterations

1. Resolve meaning from callers, strings, control flow, type and data-layout
   evidence. Document the evidence and confidence before a rename.
2. Rename fields **per owning class**, not with a global `unk_04` replacement.
   Preserve field order, types, sizes and offsets, including packed layouts.
3. Keep `config/vcl-link-order.cmake` in its recovered order whenever changing
   source filenames. Adjust `config/build.json` if a renamed file has special
   compiler flags; do not silently discard per-TU flags.
4. Preserve original mangled names of ABI-visible functions, virtual methods,
   classes and globals unless a byte-exact-compatible mechanism has been
   verified. Source aliases can be staged before symbol-level changes.
5. Treat short parameters and locals as **manual review**: `i` for a short
   loop index may be fine, while `a` for a CPU descriptor or `p` for an
   instruction owner may obscure meaning.
6. Verify 13 modern CTest cases plus the historical `exact-sha256` test after
   each accepted batch. Reject or revert any naming change that alters
   historical bytes until the cause is understood.
7. Keep the VUC reconstruction independent and avoid rewriting its incomplete
   original ABI assumptions without source/relocation evidence.

## Next targets

- Owner-qualified `LocTag` and `LocRef` fields, especially source text,
  location stack and line numbers: inspect every use before renaming common
  field spellings like `unk_00` across unrelated classes.
- `Instr`, `instStream`, `workTable`, `Program` and `Processor`: largest
  unresolved field clusters; investigate owner-qualified fields using the AST
  field inventory rather than guessing from offsets.
- No address-coded source filenames remain. When revisiting translation-unit
  boundaries or renaming descriptive names, preserve the historical order,
  special flags and ELF byte identity.
- The VUC `zzzzMakeRTTIWork` method: wait for verified vtable/RTTI semantics;
  do not assume it can be renamed without affecting its exact match.

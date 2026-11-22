# Naming audit: VCL and VUC reconstruction

Status: iterative semantic naming pass, 2026-11-22.

This work inventories opaque identifiers and improves them **without changing**
the verified VCL executable. It is deliberately separate from functional
reverse-engineering: a plausible name does not prove a function or field's
meaning, and a name-only change must not change instruction behavior, object
order or output bytes. The linker's symbol spellings may change when the
historical stripped ELF remains byte-identical.

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
| Address-named functions | 436 | Original baseline before semantic function renames; see the progress report for remaining names |
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
  `src/f080843a0.cpp` -> `src/decodedLineQueries.cpp` (the latter was
  further renamed in the decoded-line naming pass).
- Recovered function names also include `abortAfterError`,
  `asciiCaseEqual`, `streamName`, and `registerName`.

These names are now used **directly in C++ declarations, definitions and
callers**, without `#define` function aliases. A full historical GCC rebuild
verified that changing the mangled names of these non-virtual functions does
not alter the stripped ELF SHA-256. The opaque class names still have semantic
typedefs because changing a class name can have different ABI consequences.

## Second accepted rename batch: parser and source units

- `Parser` fields: `locationTagCount`, `pendingIncludeName`,
  `pendingMacroName`, `gaspLocationStack`, `gaspLocationDepth`,
  `rawBuffer`, `processedBuffer`, `parsedLineCount`, `parsedLines`.
  The names follow their allocation, lifetime, and use in `Parser.cpp`.
- `LocTag` fields: `tokenCount`, `tokens`; `TokenEntry` fields:
  `textOffset`, `separator`, `inComment`. These follow the tokenizer's
  writes and token lookup; the structure layout and field types are unchanged.
- Ten parser/line-entry methods now have descriptive **C++ names**:
  `initializeParser`, `buildProcessedLines`, `getParsedLineCount`,
  `getParsedLine`, `addLocationTag`, `decodeGaspLine`,
  `isAssignmentLine`, `tokenizeSourceLine`, `tokenTextAt`,
  `dumpSourceLine`. These are direct function names, not preprocessor aliases.
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

## Post-merge parser and location naming

The local parser refinement was reapplied on top of the merged naming PR
without reverting its wider field and filename renames:

- `Line` owns `locationName` and `locationKind` (file or macro), with semantic
  methods `setLocation`, `clearLocation`, `dumpLocation`.
- `LocTag` owns `lineText`, `sourceLineNumber`, `isRawMacroBlock`, `gaspDepth`
  and `gaspStack`. `LocRef` entries hold `sourceLocation` and
  `gaspLineNumber`. The parser's GASP location-stack writer and the line-dump
  reader independently confirm these interpretations.
- Five additional parser helpers now read `isLocTagValid`,
  `getLineTokenCount`, `startsWithDirective`, `isCommentToken` and
  `hasAssignmentMarker` at their call sites. The methods are now directly
  renamed in declarations and definitions.

The nine corresponding owner-qualified field review entries were removed from
`naming-fields.tsv`, with already-resolved short Line declarations removed
from the separate AST review queue. The generated progress report reflects
the merged branch rather than the older local source snapshot.

## Third focused batch: move records and operand-location lists

The move-record free-list path in `getMove` / `addFreeMove` establishes five
`moveRec` fields unambiguously: `nextFree`, `moveType`, `instruction`,
`moveRegister`, and the four-entry `partLocks` array. The instruction and
register pointers are checked against the cached move record before reuse.
Their order, offsets and types are unchanged.

The operand-location list's mutation paths establish its `head` and `count`:
the add/insert functions update both, the clear function resets both, and the
diagnostic visitors check the actual traversal length against `count`.
Eight `olList` operations now have semantic C++ names:
`validateOperandLocations`, `dumpOperandLocations`, `addOperandLocation`,
`addInstructionLocation`, `copyOperandLocation`, `firstInternalLocation`,
`firstExternalLocation`, and `insertExternalLocation`. The list node type
`ol_s` remains distinct from the containing `olList`; its similarly named
fields were not touched.

These seven owner-qualified field declarations were removed from the field
review queue. The updated progress report counts 40 direct function renames
relative to the original 436 address-based function names.

## Decoded source-line and operand-descriptor naming

The old `flowBlock` type was a provisional name for a **decoded source-line
entry**, not an actual control-flow basic block. The decoder creates one entry
for a raw pass-through block, a synthetic instruction, or a UTA/RAW line; the
stream splitter then assigns a flow ID and groups entries. It is now named
`DecodedLine`, with its implementation in `decodedLine.cpp` and the query
helpers in `decodedLineQueries.cpp`. The original translation-unit order is
unchanged in `config/vcl-link-order.cmake`.

Eight owner-qualified `DecodedLine` fields now follow the evidence in
`oDecode.cpp`, `splitParserStream.cpp`, and the stream builders:

| Previous field | Direct C++ name | Evidence |
| --- | --- | --- |
| `unk_00` | `passThroughText` | Owned raw source string, transferred to the pass-through stream |
| `unk_04` | `isBranchTarget` | Marked by branch-target label resolution |
| `unk_05` | `decodeMode` | Selects UTA, RAW, synthetic and pass-through dispatch |
| `unk_06` | `decodeSubMode` | Checked during stream grouping and RAW construction |
| `unk_08` | `flowId` | Assigned during flow identification, then used to group entries |
| `unk_0a` | `rawLoopId` | Copied from the parser's active RAW loop ID |
| `unk_0c` | `instructions` | One or two decoded instructions per source line |
| `unk_14` | `labels` | Owned list used for duplicate-label checks and stream naming |

Eight numeric function names have also been replaced with direct names:
`DecodedLine::initialize`, `DecodedLine::refreshInstructionOperands`,
`DecodedLine::releaseContents`, `DecodedLine::hasFlowStartSynthetic`,
`refreshDecodedLineOperands`, `findDecodedLineByLabel`,
`maximumDecodedLineFlowId`, and `decodeSourceLines`.

The `iOpDesc` operand descriptor now exposes five independently verified
fields: `operandKind` (zero terminates the descriptor array), `direction`
(1=input, 2=output), `isImplicit`, `outputLatency` (copied into the output
operand's scheduling latency), and `patchedOutputIndexPlusOne` (the one-based
output operand referenced by an implicit-input patch). The
`iInfo::operandDescriptors` array is also named directly. The two remaining
opaque `iOpDesc` bytes were intentionally left unresolved: their meanings
are not yet proven by the current callers.

This pass resolves 13 further owner-qualified fields and eight address-based
function names, without source-level `#define` function aliases or changes
to struct field order, field types, alignment, or the historical executable.

## Opcode metadata and intrusive list naming

The opcode lookup, parser, instruction emission, and EFU hook paths establish
eight more fields on `iInfo`:

| Original field | Recovered name | Evidence |
| --- | --- | --- |
| `unk_04` | `mnemonic` | Lookup-table terminator and text emitted for the instruction |
| `unk_0c` | `destinationMaskMode` | Controls accepted `.xyzw` suffixes and emitted destination-mask formatting |
| `unk_18` | `kindAndFlags` | Low three bits encode `kind()`; higher bits are also tested |
| `unk_19` | `kindData` | Kind-dependent branch flags, synthetic kind, or load/store type |
| `unk_1c` | `efuMaskType` | EFU hook selects fixed component masks from this value |
| `unk_20` | `decodeHook` | Invoked by the instruction decoder with an `Instr *` |
| `unk_34` | `reverseSenseMnemonic` | Selects the opposite branch sense during emission |
| `unk_38` | `emitHook` | Emits the instruction through a per-opcode callback |

Five of the type's numeric helper names now read `branchFlags`,
`syntheticKind`, `isRawOnly`, `isNop`, and `canPruneOperands`. Their result
conditions and callers establish these interpretations. Other metadata bytes
and methods stay opaque where a semantic interpretation is not established;
for example, `unk_1a` is interpreted with different indexing conventions in
the branch and effective-address paths.

The intrusive list implementation also now uses direct semantic names for
`mList`'s `nodeSerial`, `nodeCount`, `head`, and `tail`. Its `mNode` links and
contents are `next`, `previous`, `ownerList`, `data`, `name`, and `flags`.
The generated serial is stored in `mNode::data` as a pointer-sized token;
`mList::findByData` also searches this same field. The append/prepend/detach
methods prove the link ownership and count invariants, while the node-name
and flag readers establish the other fields.

Fourteen `mList` methods were renamed directly (including `clearNodes`,
`appendNamedNode`, `findByExactName`, `findByNameIgnoreCase`, `appendNode`,
`detachNode`, `transferNodesFrom`, `combinedFlags`, and
`takeCombinedFlags`), along with `mNode::clearNameAndOwner`.
The traversal-only `mList::func_0804b534` remains numeric because the
recovered body does no observable work; assigning a purpose to it without
evidence would be misleading.

This pass resolves **19 owner-qualified fields and 20 direct function names**.
No C++ source-level function-name macros, layout changes, or algorithmic
changes were introduced. The historical GCC build and the original stripped
ELF SHA-256 remain byte-for-byte matched.

## Extended core-types naming pass: diagnostics, register pools and scheduling

This deliberately larger review unit groups several related C++ source
renames. **None** are implemented by preprocessor aliases. Changes were
accepted only after a native build and a historical byte-exact rebuild; the
original stripped ELF SHA-256 remains
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.

### Instruction diagnostics and used-before-set information

- `iHolder` fields now identify `instruction`, `operandIndex`, and
  `componentMask`; its two numeric helpers are `matchesOperandLocation` and
  `dumpOperandLocation`. These are grounded in the `ol_s` instruction, operand
  and component references passed to `Instr::leDump`.
- `infoList` now has `head` and `count`, with a `dumpEntries` traversal;
  `infoNode::next` is its single linked-list pointer.
- `stallNode` now exposes `isLocal` (comparing stream flow IDs), `setter`,
  `user`, and `stallByType[3]` (latency, throughput and register-file-write
  stalls). The raw/UBA stall loader reads the same typed array.
- `ubsNode` identifies the `reg`, `slotMask`, `entryStream`, and `useStream`
  recorded when an input is used before its register is set.

### Register and work-table metadata

- `regUseList` owns `count`, `head` and `tail`. Its proven operations include
  `hasConsistentCount`, `hasEntries`, `findByRegister`, and per-class/all-pool
  diagnostic dumps. `regUse::validateAndDumpParts` checks recorded throughput
  information before reporting component-pair data.
- `wtRow` owns the two `entries`, `skip`, `maxStallLength`, `stallTypeMask`
  and `cycleTime`. The add/clear stall paths and the instruction emitter use
  these exact members. Its input/output mask operations now also have direct
  semantic names, as do their underlying `Instr` queries.
- `wtInstrList` has `head`, `tail` and `count`, verified by the intrusive
  add/remove operations in `lists.cpp`. These are shared by the linear,
  pending and completed instruction queues; their field order is unchanged.
- Twelve `RegPool` helpers now have names for index and name lookup,
  physical/virtual register discovery, creation, cleanup and cycling,
  register-class ownership, and marked-register counting. Ambiguous
  reset/dump functions were not renamed merely for higher coverage.

### Processor parsing state

Sixteen high-confidence `Processor` fields are now named from the parser,
directive and emitter paths: `newSyntax`, `exportedSymbols`, `globalName`,
`cpuName`, `trailingLabels`, `decodeMode`, `decodeSubMode`, `insideRawLoop`,
`rawLoopId`, `mpgTargetAddress`, `visibilityScopes`, `uniqueAll`,
`savedDecodeSubMode`, `savedDecodeMode`, `usedBeforeSet` and `stalls`.
The saved-mode fields are named for their **restore-site roles**, without
claiming that the recovered implementation necessarily stores both modes at
every entry point. Other unexplained processor bytes remain opaque.

Together these batches resolve **35 further numeric function names and 41
owner-qualified field names**. The generated progress inventory now records
103 directly renamed functions out of the original 436 and 135 resolved
offset-style fields out of 614. Function bodies, field types, struct layouts
and historical translation-unit ordering have not been intentionally changed.

## Extended instruction and stream reconstruction pass

This larger pass groups **89 direct function renames and 68 owner-qualified
field renames** across the instruction, operand, stream, link-graph,
work-table, register-pool and opcode-access subsystems. It was performed on
`refactor/semantic-naming-instruction-stream` after the preceding core-types
PR was merged. No function-name preprocessor aliases were introduced.

### Instruction and operand lifecycle

- `Instr::validateOperands`, `isNop`, `getOwningStream`,
  `resetFlowDependencies`, `clearTransientFlags`, and
  `resetOptimizationState` are named from their function bodies and callers.
- The input and output operand queries now expose direction explicitly:
  `findInputOperand`, `findOutputOperand`, `collectInputRegisterMask`,
  `collectOutputRegisterMask`, `hasLinkedOutputFrom`, and
  `getFirstOutputLatency`. The immediate getter is `getImmediateOperand`.
- `patchBranchLatencies`, `simplifyBranch`, and `normalizeZeroSources`
  identify three separate branch/dependency-normalization operations.
- Operand handling now provides `validateKindAndMask`, `diagnoseOperand`,
  `resetFlowState`, `resetOptimizationSlots`,
  `emitExpectedOperandType`, and `getRegisterNameOrNone`. The slot helpers
  reset locks or clear operand locations, without changing their bodies.
- Seventeen instruction fields were renamed from proven initialization and
  use paths: `branchFlags`, `rawOnly`, `destinationMask`, `scheduledCycle`,
  `predecessorDependencies`, `successorDependencies`, `syntheticKind`,
  `ramAccessType`, `opcodeFlags`, `addressImmediateValue`,
  `linearOrderIndex`, `position`, `hasExtraPredecessors`,
  `specialRegisterReadMask`, `specialRegisterWriteMask`, `parsedLoopId`,
  and `previousThroughputWriter`. For example, the forward-dependency
  constructor writes predecessor/successor lists as a symmetric pair; the
  instruction position is read by the insertion, scheduling and emitter
  paths. Remaining `Instr` offset names were not guessed.

### Instruction streams and graph links

- The stream record describes incoming/outgoing graph links, source and
  emission tables, back-trace state, and the flow's branch/start/end types.
  Thirty fields now identify this information directly, including `labels`,
  `passThroughText`, `inputLinks`, `outputLinks`, `sourceTable`,
  `optimizedTable`, `emissionTable`, `savedTable`, `visitState`,
  `traceStore`, `backtraceDependencyLength`, `streamId`, `streamSerial`,
  `startType`, `endType`, `branchType`, `knownMinimumLoopCount`,
  `safeMinimumLoopCount`, `dependencyLength` and `logicalLength`.
- Stream methods now name the two RAW layout forms, scheduling modes,
  recorded and clamped dependency lengths, forward-dependency building,
  record copying, and start/end validation. These map directly to the mode
  tests and link/dependency construction in `instStream.cpp`,
  `gatherStats.cpp`, and `fDepClone.cpp`.
- The intrusive stream graph uses `linkList::{head,tail,count}` and
  `linkNode::{previous,next,otherStream,active,linkType}`. Constructor,
  unlink, relink, graph traversal and link-type matching prove these names.
  The still-undecoded `linkNode::unk_10` is deliberately not conflated with
  `active` (`unk_11`). Link searches and marked-link counts have semantic
  method names.

### Work-table rows and register-pool lifecycle

- `workTable` now names `linearInstructions`, `pendingInstructions`,
  `completedInstructions`, `vectorMoveCount`, `oddPipeMoveCount`,
  `evenPipeMoveCount` and `instructionCountsByPipe`; their queue and count
  relationships are visible in `lists.cpp`, `iTable.cpp` and the move
  insertion paths.
- Accessor/helper names include the inclusive row lookup,
  `getRangeStart`, `getRangeEnd`, `getLoopSize`, RAW scheduling mode
  selection, the conditional branch query, and flow-stall reset.
- `RegPool` has `registerClass`, `virtualHead`, `virtualTail`,
  `virtualCount`, and `registerPrefix`; `RegPoolEntry::nameAliases` owns the
  list used by both name lookup and register-name emission. The constructor
  and virtual-register add/delete traversal provide the ownership evidence.
  In `RegPool::Init`, the prefix assignment must explicitly use
  `this->registerPrefix = registerPrefix` to avoid shadowing by the parameter.
  A historical rebuild caught the accidental self-assignment during review;
  fixing it restored the original executable SHA-256.
- Register-part methods now have names for pending-location allocation,
  state/timing reset, payload reads/writes, part cycling, register aliases
  and clearing payload markers. The older and identical but distinct
  reset functions remain numbered until their separate call semantics can
  be established.

### Opcode lookups and retained ambiguity

`getOpcodeInfoById`, `findOpcodeInfoByMnemonic`, `getOpcodeId`,
`findRegisterByGlobalIndex`, `isVuRegister`,
`findAddressAliasDescriptor`, and `findAddressAliasUse` directly express
their table-lookup behavior. The stat-counter methods now say whether they
invalidate, zero, copy, or merge maxima. Methods whose bodies are empty or
whose meaning remains ambiguous were intentionally left numbered.

After this pass, the fixed-baseline report records **192 of 436 directly
renamed functions** (244 still address-named) and **203 of 614 resolved
offset-named fields** (411 still unresolved). Changes to actual instructions,
ABI layouts, translation-unit ordering, and VUC were not intended. The
historical stripped executable still matches SHA-256
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.

## Extended graph, scheduling, register and lock naming milestone

This review deliberately groups six related areas into one PR after the
instruction/stream work in merged PR #6. It resolves **52 direct C++ function
names and 142 owner-qualified offset-style fields** in the curated inventory;
an additional `ol_s` union alias was renamed but was not a separate item in
that inventory. No function-name `#define` aliases, structural reordering,
field type changes, or new translation units were introduced.

### Program and stream graph ownership

- `Program::symbolicOutput`, `traceStoreEnabled`, `traceTargetStream`, and
  the four stream worklists follow their parser, backtrace, and graph
  traversal callers. Stream linkage now names `nextSourceStream`,
  `nextMatchedStream`, `nextResolvedStream`, and `nextActiveStream`.
- The previously accepted `Program::oddPipeMoveCount` and
  `Program::evenPipeMoveCount` were misleading: these members are not the
  counters on `workTable`. Their call sites prove
  `trackMaximumPathLength` and `traceStoreWordCount`, respectively.
- `Program::decodeAndSplitStreams`, `hasStreamLabel`, and the exported-symbol
  query are named for the operations performed in their bodies.

### Work-table scheduling, instruction records, and operands

- `workTable` fields now expose `allocationMode`, `codeModeValue`,
  `rawSchedulingMode`, `ownerStream`, `registerUses`, `registerLockLists`,
  `rowCapacity`, `rows`, `rangeStart`, `rangeEnd`, `loopSize`,
  `mainLoopId`, `externalMinimumLength`, `moveListHead`, and `statistics`.
  All of these retain their original sizes and offsets. The `Block`
  subclass and raw-table insertion, emission, and allocation paths provide
  independent evidence of the fields' uses.
- Additional `Instr` names distinguish `permittedPipelineMask`,
  `schedulerCategory`, `branchLabelTags`, insertion/serial fields, the
  address-alias mode and operand index, dynamic timing constraints, and
  abstract/concrete move metadata. `Instr::classifySchedulingCategory`,
  `emitExpectedSyntax`, and `opcodeId` follow the recovered implementations.
- `Operand` now exposes the kind, register directions and masks, implicit
  status, original/assigned/forced register references, immediate value,
  paired operand index, and per-component `parts`. The three register
  pointers are deliberately distinct: call sites retain the original
  symbolic names separately from assigned and forced registers.
- `ol_s` now identifies its next link, instruction, path key, operand index,
  component index, combined operand/component key, and state flags. Its
  ownership helper names include `nextInternalLocation`,
  `nextExternalLocation`, `operandAtLocation`, `registerAtLocation`,
  `sameOperandLocation`, and the matching stream/row/serial accessors.

### Register webs, register uses, and scheduling locks

- `RegWeb` fields now identify assigned, virtual and forced registers,
  parent web, member count, web index, and component mask. `RegPoolEntry`
  provides move-row bounds, its previous link, per-register serial, move
  instruction, assignment-cost size, pending-assignment link, and four
  `parts`.
- Each `rPart` holds a current location, pending source, cycle age,
  issue lock, per-part payload, and pending locations. These identities
  follow the register lifecycle, emulation, and throughput scheduling paths.
- `regUse` fields now express its linked-list neighbors, register entry,
  external/input/output/throughput masks, hazard masks, block mode, defining
  instruction, and four `slots`. Each `regUseSlot` has location lists for
  inputs, outputs, sources and external references, throughput pairs and
  summaries, external cycle bounds, timing values, assignment payload,
  and a register lock.
- The existing mangled class identities `cls0805bcc4` (`OperandLock`) and
  `cls0805c244` (`RegisterPartLockList`) remain unchanged. The former now
  identifies lock ownership, next sibling, parent/children, sorted output
  link, direction, latency, priority, pair, reference count, owner pointer,
  and owner operand/component/type. The latter exposes register identity,
  component index, assignment state, next lock list, output lock chains,
  counts, and outstanding locks. Its diagnostic helpers now name the
  maximum output constraint and flagged-output count. Fields with
  insufficiently proven timing semantics remain offset-named.

### Byte-exact acceptance checks

The historical RH9/GCC toolchain was rebuilt after the major field batches.
Two source-level naming hazards were specifically caught by its SHA check:

- In `workTable::doPostOpt`, local variables named `rangeStart` and
  `rangeEnd` hid the renamed members. Explicit `this->rangeStart` and
  `this->rangeEnd` restored the intended reads and original compiler output.
- In `regUse::Clone`, assigning a register through the local `reg` alias
  instead of directly from the source member changed GCC 3.2 code
  generation, even though the two expressions are semantically equivalent.
  The direct `n->registerEntry = registerEntry` dependency is required for
  the original output. A temporary self-initialization introduced during
  investigation was also corrected before acceptance.

After correcting those two sites, the stripped historical ELF is again
**byte-for-byte identical**, SHA-256
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.
The native build, 13 CTest fixtures, and naming-inventory consistency check
are the other acceptance gates. This snapshot leaves **192** address-based
function names and **269** owner-qualified offset fields for subsequent
review; neither count measures byte-matching completeness.

## Expression evaluator, optimizer and stream internals milestone

This larger review follows the merged register/operand-lock naming milestone.
It resolves **113 direct C++ function names and 83 owner-qualified offset-style
fields** (196 tracked improvements) in one coherent review. Function names
are changed in declarations, definitions and call sites directly, without
preprocessor aliases. The original object layouts, member types and
translation-unit ordering have not changed.

### Expression evaluation, preprocessing and parser utilities

- The preprocessor helpers now distinguish GCC, GASP and alternate GASP
  selection, flag updates, invocation, tool-option matching and advice about
  missing preprocessing. Parser helpers name newline detection, token
  separators, label-colon detection and keyword matching, following the
  corresponding token and line tests.
- VU string helpers now expose component-mask parsing and formatting,
  instruction modifier parsing, modifier validation, and component-bit
  selection. These names are supported by their switch cases and the
  `x/y/z/w` component masks.
- Expression tree nodes identify depth, operator type, precedence, operand
  arity and child count. The existing expression operator table and recursive
  evaluation paths establish those meanings. Tree helpers name unknown-symbol
  collection, lookups, resetting, walking and counting.
- The `mValue` arithmetic operations now identify addition, subtraction,
  multiplication, division, remainder, left/right shift, boolean and bitwise
  operators, numeric type promotion and immediate comparisons. Their actual
  `mValue` operations, not guesses based on function addresses, support the
  names. No evaluation logic was changed.

### Instruction, scheduling and stream graph ownership

- `iInfo` now distinguishes pipeline mask, resource mask, minimum issue
  spacing and branch target operand selector. The original packed union and
  descriptor layout remain intact.
- The eight `cls0805e2e4` scheduling counters now identify forward/reverse
  latency/dependency lengths and move-chain statistics. `Instr` and
  `instStream` both name their embedded `schedulingStatistics`; the statistics
  update, merge, optimizer and candidate-scoring callers provide evidence.
- `instStream` now identifies optimization records, optimization mask,
  loop region role, barrier flags, backtrace register use, source/output
  locations and shortest/longest path lengths, emitted instruction offsets,
  loop-directive counts and physical instruction budget. `istRec` fields
  reflect the optimization record emitted with stream diagnostics.
- `Instr` now identifies its clone image, instruction ID, source tag,
  linear/pending list links, next move, owning work table, replacement word,
  candidate attempt/cost/row metadata and move record. `iLink` directly names
  next/previous instruction pointers, while `InstructionPosition` names
  linear row, pipeline, loop iteration and loop-local row.
- External input/output scheduling constraints and operand locations are
  named from their setters, readers and matching lock decisions. Loop
  prologue/epilogue records now expose stream/table, loop key, source loop ID,
  index bounds and conditional branch; the matching loop-debris object
  identifies the source table and requested allocation mode.

### Pipeline entry points, resource helpers and precise validation

- Program-level operations now name flow dependency rebuilding, statistics,
  move analysis, register assignments, loop expansion, stream concatenation,
  scheduling optimization, register-cap insertion, immediate fixups and
  pruning preparation. Register-pool passes and their processor wrappers
  retain their separate first/second identities even where their bodies
  currently have the same implementation.
- Common helpers now identify bit counting, exact/case-insensitive string
  comparisons, component-suffix removal, buffer clearing, device-name
  lookup, path-key construction and backtrace state initialization. Existing
  empty helper implementations are explicitly named `noop*` rather than
  being attributed unproven behavior.
- Owner-qualified member replacements were checked across all consumers;
  a conflicting `statCounts::unk_00` reference was kept intact because it
  is not an `instStream` optimization record.

The source was rebuilt using the verified historical RH9/GCC 3.2 toolchain.
The original stripped VCL ELF remains **byte-for-byte identical**, SHA-256
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.
The final review also requires the 13 modern CTest checks, historical
`exact-sha256` check and generated naming-inventory consistency check.
The cumulative queue now contains **79** address-coded functions and **186**
owner-qualified offset fields, down from 192 and 269 before this milestone.
This measures naming coverage only, not binary reconstruction coverage.

## Directive, scheduling, lock and compiler-global naming milestone

This review deliberately groups the remaining high-confidence naming work into
one larger PR after merged PR #8. It resolves **75 numeric function names**,
**51 owner-qualified offset fields**, and **71 address-suffixed globals**
(**197 tracked naming improvements**) without using function-name `#define`
aliases. Fields retain their original order, storage types, and ABI offsets;
the source translation-unit order is unchanged.

### Directive dispatch and CLI/runtime helpers

- The 28-entry directive dispatch table directly establishes the handlers for
  `.vu`, `.mpg`, `.vsm` / `.raw`, `.endvsm` / `.endraw`, both loop delimiters,
  `.init_vf`, `.init_vi`, register removal and initialization, `.name`,
  `.syntax`, `.global`, `.unique`, alignment and end-bit options, visibility
  scopes and `.vu_work_area`. Function names now reflect those concrete
  effects. Shared handlers retain one shared C++ function name rather than
  creating separate implementations.
- The same pass identifies directive lookup, register-range handling,
  visibility-scope push/pop, memory work-area initialization, the source-file
  and descriptor readers, temporary-file creation, watchdog callbacks and
  the expression/immediate value type formatter.
- Descriptive `noop*` names are reserved for actual zero-effect helper
  bodies. Methods that always accept/reject are named for that observable
  behavior instead of hypothesizing an undocumented directive purpose.

### Scheduler, allocator and instruction flow

- Dependency-placement helpers expose earliest/latest dependency rows,
  candidate refinement, allowed-pipeline checks, insertion constraints,
  register-writer pulls, move queueing and move-list consistency. Both
  scheduling modes remain separate, with direct names at declaration,
  definition and call sites.
- `workTable` fields identify selected optimizer, declared raw length,
  register lock-list count, pending move count, last scheduled position,
  logical instruction length and dual-pipeline instruction count.
  `wrangleCand` holds its instruction, candidate row and candidate pipeline.
  `instStream` identifies the reduction-direction switches and skip state,
  while `Instr::pruningRestricted` follows the pruning checks.
- The two `RegPool` virtual methods now directly name work-table assignment
  and discarding disjoint external registers. They are renamed consistently
  in the base class and every derived override; vtable layout and method
  ordering are unchanged. `RegPool::poolKind`,
  `RegPoolEntry::enabledPhysicalRegister` and
  `Processor::vuDirectiveSeen` follow their respective construction,
  registration and directive handling sites.

### Backtrace cache, locks and ancillary records

- The six `VisitCache` fields express record data, words/bits per record,
  capacity, record count and whether recording began. Both the class methods
  and inlined record helpers confirm those identities. `statCounts` now
  names resource instruction counts; `iGroupNode` identifies its payload
  and size; work-table entry objects identify their stored instruction.
- Register lock lists now distinguish active references from total
  references, inserted status, positions and consumer counts. The timing
  fields identify earliest ready, full latency, earliest consumer-free and
  latest consumer/ready cycles, with distinct adjusted loop-relative values.
  Only counters supported by the corresponding `calcStats` computations
  were named; less certain timing fields remain offset-named.
- `Operand::reductionHazardMask`, `ol_s::cycleOffset`, and
  `linkNode::includedInPathTraversal` are traced through their consumers.
  A proposed lock field name `isInserted` collided with an existing C++
  predicate; the final direct member name is `insertedIntoLockList`.

### Global variable names and acceptance checks

- Address-coded globals now identify active scheduling bounds, VU opcode and
  register tables, processor state, diagnostic switches, compiler options,
  preprocessing tools, parser keywords, literal/operator tables,
  register-allocation grids, collapse state, scheduler watchdogs, current
  optimizer, output name and temporary formatting buffers.
- Renaming globals does not relocate or reorder their definitions. The
  historical binary check accepts only byte-identical output, not merely a
  successful compilation. The remaining 14 address-suffixed globals and four
  numeric functions are deliberately held for later evidence-based review.

Following the full modern build and historical RH9/i686 GCC 3.2 rebuild,
the stripped historical VCL ELF remains **byte-for-byte identical**, SHA-256
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.
Modern CTest (13 fixtures), historical `exact-sha256` and naming-inventory
consistency are the acceptance gates for this change. The fixed-baseline
inventory now has **4** address-named functions, **135** owner-qualified
offset fields and **14** address-suffixed globals remaining. These metrics
describe naming coverage, not how much of the executable has been matched.

## Final address-name elimination and structural field audit

The milestone following merged PR #9 eliminates all **four** remaining
address-named C++ functions and all **14** remaining address-suffixed
globals, and resolves **71** further owner-qualified offset fields. This
accounts for **89 tracked naming improvements**. The four functions receive
direct source-level names, not aliases:

- `LocTag::probeNonCommentTokenAndReturnFalse` conditionally reads a token and
  always returns false; the name describes the code rather than assuming a
  now-lost parser intention.
- `Instr::constantZeroInstructionHook` returns zero unconditionally.
- `isScalarFloatingOperandKind` and
  `isVectorOrAccumulatorOperandKind` follow the explicit sets of operand-kind
  numbers in their implementations. These predicates remain separate even
  though no additional callers currently explain their original roles.

### Field ownership and observed semantics

- The source-line decoder uses `ImplicitOp` to hold parsed register text,
  immediate text, tagged branch labels, component masks, address component
  masks, a generic-operand form and forced physical-register binding. The
  `aaDesc`/`aaUse` tables identify opcode IDs, operand indexes, address-alias
  modes and literal immediate values; their usages in `immFixup` determine
  the names.
- `Operand` and `Instr` now name source token bounds, immediate type and
  external block mode. The optimizer's paired original/optimized instruction
  image words retain their pointer-word types and exact layout, while
  `Instr` dependency reachability flags, original pipeline mask, grouped
  instructions and full-loop move-drag condition are named from callers.
- `subCache` describes its allocated table, maximum flow ID, per-row stride
  and total storage. `xpNode` tracks unmatched opening parentheses, `ol_s`
  tracks the maximum path key, and `instStream` records branch forward
  latency depth. The modern and historical builds check these renamed types
  against the original data layout.
- `workTable` now identifies insertion serials, loop bounds, dynamic start
  rows, fallback scheduling controls, stream lengths, post-optimization state
  and a potential loop boundary instruction. `regUse` identifies the path
  hazard mask; `rPart` distinguishes emulation timing metrics and pending
  read-cycle bounds.
- The lock system now identifies per-pipeline input demand and aggregated
  consumer demand, loop-wrap lower bounds, and output-lock conflict counts.
  Register descriptors and entries expose their hot-output-path setting and
  transient input component mask; the allocator records per-register-part
  availability. Existing names with uncertain semantics are not forced.
- The paired-operand and count override descriptors now expose operand
  indexes and count fields without changing the positional initializers of
  the opcode tables. `opPair_s::pairingMode` is a provisional semantic name
  for the mode tag and should be revisited if a consumer is recovered.

### Remaining opaque areas and verification

The 14 renamed globals cover instruction serial allocation, pooled move
records, cap insertion, loop locking, reserved instruction keys, scheduler
pass counters, diagnostic options and immediate formatting. The 64 remaining
owner-qualified `unk_*` fields are disproportionately ABI padding, reserved
opcode metadata, unexplained array payloads and compiler-specific state.
They are left unchanged rather than being assigned unsupported meanings.

The new fixed-baseline inventory is **0** address-named functions (from 436),
**0** address-suffixed globals (from 85), **0** address-named source files
(from 16), and **64** owner-qualified offset fields (from 614). These are
identification metrics, not a statement about reconstruction completeness.

The historical RH9/i686 GCC 3.2 build and `exact-sha256` test preserve the
stripped VCL executable exactly at SHA-256
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.
Modern CTest and generated naming-inventory checks are also required.

## Scoped parameters and local variables: directive, flow, processor and value review

Following the merge of PR #10, source review shifted away from completed
address-based identifiers to genuinely useful **function-scoped** names. This
milestone applies **284 direct C++ identifier renames** in four core
implementation files, and updates their public declaration parameter names
where applicable. The original AST review queue loses **131 parameter** and
**143 local-variable** entries (274 tracked resolutions); ten additional
identifiers were improved in those same scopes even though they were not in
the original short-name queue.

### Evidence and naming decisions

- **Directive handlers (`ExecRP.cpp`, 62 renames):** all short `LocTag *t`
  handler parameters become `sourceLine`. Directive-specific tokens identify
  a program name, MPG address, exported symbol, reorder option, syntax option
  or work-area address/size. In register-range parsing the input/output
  cursors, token index, register entry and swapped bounds are distinct.
  Visibility-scope push/pop uses `visibilityMode` and `scopeNode`.
- **Program/flow (`Program.cpp`, 52 renames):** `Program *p` becomes `program`
  and instruction-stream arguments/iterators become `stream`. Stream
  retargeting distinguishes incoming/outgoing links, predecessor stream and
  branch instruction. Decoding distinguishes build type, decoded lines and
  split result. Emission names optimizer indices, output cursor and emitted
  instruction count; backtrace/flow routines identify label nodes, stream
  indices and path links. `Program.h` is kept consistent for affected APIs.
- **Processor (`Processor.cpp`, 61 renames):** register-pool initialization
  distinguishes descriptor indexes and register entries. Pipeline pairing
  uses `pipelineEntries`, `pipeIndex`, `pairedPipeIndex`,
  `pairedInstruction` and the two output registers. Opcode lookup and
  address-alias search identify opcode info, input/mnemonic cursors,
  descriptor candidates and opcode IDs. Pool lookup identifies class,
  output capacity and matched counts rather than reusing `p`, `r` and `n`.
- **Typed expression values (`mValue.cpp`, 109 renames):** arithmetic and
  comparison consistently distinguish `leftValue`/`rightValue`, signed,
  unsigned and floating conversions, and the selected `promotedType`.
  Literal parsing, numeric conversion and immediate formatting identify
  their source and buffer cursors. Matching public declarations in `mValue.h`
  use the same operand names. Existing order, promotion, conversion and error
  handling are preserved, including unused/unreachable locals.

### Reproducibility and boundaries

Each rename was applied within its specific function body by a lexical
scanner that preserves strings and comments; affected declarations were
removed from the *original, owner/function-qualified* AST review queue. This
avoids the unsafe alternative of replacing single-letter names globally.
There are no alias macros, structural field changes, parameter ordering
changes, code-path rewrites, source-file moves or VUC branch changes.

The current fixed-baseline inventory retains **734** short/generic parameter
review entries and **2,103** local-variable review entries, down from 865 and
2,246 just before this change. These queues flag potential review work,
not proof that every remaining short name should be replaced. The 64 unknown
owner-qualified fields also remain unchanged, pending substantive evidence.

Acceptance requires the native build and 13 CTest fixtures, historical
RH9/i686 GCC 3.2 rebuild and byte-exact `exact-sha256` test, plus the
generated naming inventory consistency check. The reference stripped ELF
SHA-256 is
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.

## Core pipeline scoped naming: registers, dependencies and scheduling

Following the merged directive/flow/value naming PR, this milestone expands
the review to **29 VCL implementation files** and performs **595 direct
function-scoped identifier renames**. The reviewed files cover instruction
decoding and manipulation, register-pool management, lock propagation, operand
dependencies, immediate fixups, stream/loop bookkeeping, scheduling candidate
selection, and final work-table emission. The fixed AST review queue shrinks
by **184 parameter entries** and **511 local-variable entries** (695 reviewed
declarations, because some original names occurred in multiple lexical
scopes). No numeric or alias-style function definitions are introduced.

### Source-backed terminology

- **Instructions, operands and register pools:** `Instr`, `Operand`,
  `RegPool`, and related code now distinguishes `instruction`, `operand`,
  `registerEntry`, `cloneInstruction`, `branchInstruction`,
  `sourceOperand`, `destinationOperand`, and `opcodeInfo` where supported by
  the types, register lookup and link structure. These changes affect local
  declarations and call sites, not the classes' binary layouts.
- **Dependency and lock analysis:** `setLocks`, `checkInputLock`, `catLocks`,
  `fDepClone`, `gatherStats`, and `lock` now identify `lock`, `lockList`,
  `registerUse`, `operandLocation`, `inputSlot`, `outputSlot`,
  `sourceInstructions` and `pipelineEntries`. The names reflect physical
  inputs/outputs, tracked register uses and lock-list ownership.
- **Instruction streams and loop transformations:** `iTable`, `instStream`,
  `loopDebris`, `iGroupList`, `prune`, `collapse`, `makeLpWork`,
  `getUninserted`, and `immFixup` distinguish stream/table owners, prologue
  and epilogue regions, pipeline entries, candidate rows, source locations,
  alias descriptors and immediate values. Distinct variables that refer to
  different rows or regions remain distinct even if their original short
  names were reused in adjacent scopes.
- **Scheduling, resource timing and allocation:** `wrangle`, `wrangleCore`,
  `wtQueue`, `dynCalcStats`, `externalStreamPreparation`,
  `assignLocalRegisters` and `genRegPoolCtor` identify candidate instructions,
  insertion tables, locks, register parts, dependency rows and the source
  instruction stream. Simple loop indexes with already clear meaning are
  intentionally retained unless a more specific role is established.

### Historical compiler-sensitive name collisions

This review used direct identifier replacement **within a specific C++
function**, rather than replacing short names globally. Even scoped renames
can silently change C++ meaning if the proposed local name shadows a member
or an enclosing local. The modern compiler alone cannot detect these bugs.
The historical RH9/GCC 3.2 `exact-sha256` check exposed several such cases,
which were isolated by compiling individual changed translation units:

- The `Instr::getBranchTargetImmediate` and `getBranchTargetRegister`
  temporary opcode metadata pointer is `branchOpcodeInfo`, not
  `opcodeInfo`: the latter is the original owning field.
- `cls0805c244::setRepReg` uses `selectedRegister` for the argument,
  preserving checks and writes to the separate `registerEntry` field.
- `iGroupLog::logTable` uses `sourceTable` for its incoming work table,
  preserving the independent class `table` field used for an existing-state
  guard.
- `externalStreamPreparation::removeNops` uses separate `followingRow`,
  `scannedRow`, `candidateRow`, and `sampledRow` cursors. In particular, an
  existing distance check intentionally reads the *outer* `row` while the
  inner instruction lookup uses `sampledRow`. Conflating these two produced
  a compiling but byte-different program.
- `workTable::setSkip` uses `targetRow`, avoiding collision with its integer
  `row` argument; `instStream::gatherStats` uses `streamStats` for the
  temporary pointer without shadowing `schedulingStatistics`.

The first ten files were tested individually, then the 19-file follow-up
was bisected and its remaining candidates also checked individually.
Finally the complete 29-file change set was compiled together. Both the
modern build and the historical byte-exact gate pass, with the reference
stripped SHA-256 preserved as
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.

### Queue and remaining work

The fixed-baseline review now contains **550** flagged short/generic
parameters and **1,592** flagged local variables, down from 734 and 2,103
immediately before this milestone. The **64** owner-qualified offset fields
remain untouched pending positive evidence; a zero address-coded function
count is unchanged. These are manual review metrics, not a claim that the
original compiler's source is perfectly recovered. The separate VUC branch
and original historical translation-unit ordering were not changed.

## Support structures and remaining scoped names

After merging the core-pipeline review (PR #12), the next pass extends
function-scoped semantic naming into **75 VCL implementation files**. It
replaces **449 short/generic parameter and local declarations** at **2,202
referenced occurrences**, using Clang declaration identities rather than
unqualified text replacement. The existing AST review queue shrinks by
**207 parameters** and **242 locals** (449 total). This includes a second,
focused pass on the major pipeline files, but the bulk of the work covers
supporting utilities that had not received the same scrutiny.

### Meaning established by source use

- **Program and stream graph:** doBackTrace, pathKeys, linkList,
  processActiveList, catStreams, tidy, and resolveStreamBranches
  distinguish program, stream, link, registerUse, and workTable by the
  objects they traverse and mutate. A stream-link list node is named
  link, while its destination stream is stream; the distinct ownership
  roles are not merged.
- **Operand and instruction helpers:** olList, infoList, oDecode,
  bindDependencies, regUse, and emuInst distinguish instruction,
  operand, location, registerEntry, and registerUse. These names
  follow instruction/operand ownership and typed register-use records,
  without renaming any public ABI type or altering data layout.
- **Output and string utilities:** lists and fileEmit now use
  instruction for linked instruction nodes, and more specific
  sourceCursor, destinationCursor, lineCursor, modifierSource,
  modifierDestination, sourceEnd, and paddingIndex for copying and
  formatting. These cursor roles come directly from which buffer is
  read or written in each loop.
- **Remaining optimizer and scheduling support:** findCap,
  getNumOptimisers, optHDown, optVuta, optNormal2, regRefLiveRange,
  wtQueue, RegPool, and prune use typed instruction, stream, register,
  work-table and operand names where one unambiguous declaration can
  be distinguished from the others.

### Collision discipline and limits

The pass changes individual VarDecl/ParmVarDecl identities and their
DeclRefExpr uses, not every identical spelling in a file. It skips a
candidate when the descriptive name is already used in the enclosing
function, when two candidates would receive the same new name, or when
the source/declaration mapping is ambiguous. The fixed review queue
predates some method renames, so unique source-line/name/type matches
were accepted only when they still refer to the same parsed declaration.

This is **naming work only**: it introduces no function-name #define
aliases, does not change translation-unit order, and leaves the 64
unresolved owner-qualified fields alone rather than guessing their
meaning from offsets. Short loop indices and other concise but clear
names remain unchanged. The native 13-case CTest suite, historical
RH9/i686 GCC 3.2 rebuild, and stripped exact-sha256 gate confirm
preserved behavior. The reference SHA-256 remains
83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274.

## Deeper scoped-index and text-role review

After PR #13 merged, the next review expands to **105 VCL translation
units** and directly renames **498 parameters and locals**, together
with **2,353 use-site references**. The fixed AST review queue
decreases by **119 parameter entries** (343 to 224) and **379 local
entries** (1,350 to 971). The scanner treats these as declaration-level
review items, not proof that every remaining short name is incorrect.

### Verified roles

- **Collection indices:** Short integer loop variables are renamed when
  their actual subscripts identify a unique role: operand descriptors
  and operand arrays use operandIndex, register parts and slots use
  partIndex, work-table rows use rowIndex, processor register pools use
  poolIndex, parsed lines use lineIndex and register entries use
  entryIndex. If a variable indexes conflicting collection kinds, the
  automated candidate is rejected rather than assigned a generic name.
- **Instruction links and dependencies:** Instr::link_oParts now
  distinguishes first/second operand slots, input/output operand
  slots, paired locations and their component indices. The output
  cycle offset is named clampedCycleOffset, matching its 0xff clamp.
  The dependency builder's forceDependencyChecks flag follows its
  call into BuildfDepList's force parameter; the unused second flag
  remains untouched.
- **Scheduling and output helpers:** workTable pointers with unambiguous
  local roles are called scheduleTable. Related classes identify
  candidate instructions, decoded lines, register references, and
  operand slots by type and actual consumer. Source and destination
  string cursors are distinguished in text copying, immediate parsing,
  modifier formatting and label extraction.
- **ABI-sensitive boundaries:** Class names, struct sizes, field
  offsets, overloaded function spellings, external interfaces, and
  source-file link order are not modified. This remains an
  identifier-only pass.

### Verification and skipped candidates

Names were changed using Clang declaration IDs plus direct references,
with exact source-position and source-token checks. Proposals were
skipped when no concrete collection/text role was established, when
the chosen name already appeared in a function, or when multiple
declarations would receive the same name. Common short loop
counters were retained when their domain was unclear.

The macOS CMake build and **13/13 native CTests** passed. The
historical RH9/i686 GCC 3.2 build and the exact-sha256 CTest also
passed. The stripped VCL executable retains reference SHA-256
83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274.
There are still **64 unresolved owner-qualified offset fields**;
none was renamed without owner-specific evidence.

## Rules for future iterations

1. Resolve meaning from callers, strings, control flow, type and data-layout
   evidence. Document the evidence and confidence before a rename.
2. Rename fields **per owning class**, not with a global `unk_04` replacement.
   Preserve field order, types, sizes and offsets, including packed layouts.
3. Keep `config/vcl-link-order.cmake` in its recovered order whenever changing
   source filenames. Adjust `config/build.json` if a renamed file has special
   compiler flags; do not silently discard per-TU flags.
4. Prefer real, meaningful function names instead of `#define` aliases. Verify
   the historical SHA after renaming. Be especially careful with virtual
   methods, class names and exported ABI surfaces: they may affect output even
   when non-virtual function renames have been proven byte-identical.
5. Treat short parameters and locals as **manual review**: `i` for a short
   loop index may be fine, while `a` for a CPU descriptor or `p` for an
   instruction owner may obscure meaning.
6. Verify 13 modern CTest cases plus the historical `exact-sha256` test after
   each accepted batch. Reject or revert any naming change that alters
   historical bytes until the cause is understood.
7. Keep the VUC reconstruction independent and avoid rewriting its incomplete
   original ABI assumptions without source/relocation evidence.

## Next targets

- Remaining owner-qualified fields in types unrelated to `LocTag`/`LocRef`:
  inspect their use before renaming common offsets such as `unk_00`.
- `Instr`, `instStream`, `workTable`, `Program` and `Processor`: largest
  unresolved field clusters; investigate owner-qualified fields using the AST
  field inventory rather than guessing from offsets.
- No address-coded source filenames remain. When revisiting translation-unit
  boundaries or renaming descriptive names, preserve the historical order,
  special flags and ELF byte identity.
- The VUC `zzzzMakeRTTIWork` method: wait for verified vtable/RTTI semantics;
  do not assume it can be renamed without affecting its exact match.

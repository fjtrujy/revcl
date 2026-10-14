# VUC: statement-list fold partial and fragment ABI layout

Date: 2026-10-14. Branch: `vuc`. Starting HEAD:
`7297929`. Original is the archived VUC 2004.1 ELF32 i386.
Compiler: pinned GCC 3.3.3 (2004-01-25 prerelease) with GNU as 2.15
inside the existing `revcl-oracle` Lima guest.

## Newly tracked experimental function: 60/119

The previously unresolved
`Translate(ASTNodeList<ASTStmt*>&)`, original `0x0806ada0`,
**119 bytes**, now has a separately and independently checked
**60/119-byte** partial.

- Source: `vuc/probes/translate_stmt_list_partial_oct14.cpp`.
- Manifest: `vuc/config/translate_stmt_list_partial_oct14_probe.json`.
- Compiler: `-O2`, without explicit register renaming.
- **3/3 actual original relocations**: `Translate(ASTStmt*)`
  (`0x0806af20`), `operator new(unsigned)` (`0x080499ec`),
  `IR::Seq_Data` vtable (`0x080760d8`). No fabricated addresses.
- Strict checker: `length=119`, `exact_bytes=60`,
  `exact=false`, with differences beginning at **+2**, **+24**.
- The true semantic graph processes statements, omits null results,
  returns the first surviving fragment unchanged, and subsequently
  allocates a polymorphic, 16-byte `IR::Seq_Data` combining the old
  accumulated result with the translated child. Constructor/tag, list
  entries, and named original calls are source-based.

The original has a stack-resident accumulator and keeps the input
list in EDI and the index in ESI. Previous natural `for` source emitted
only 116 bytes, keeping the result in EDI. This saved C++03 source,
using a prediction hypothesis on the loop and both nonnull tests,
recovers the **119-byte** emitted length and the stack-resident
accumulator; it does **not** recover the original initial compare
register or early-exit/basic-block order. These
`__builtin_expect` hints are layout probes, **not proven original
authorship**.

Two independent, original-ELF-aware matrices were executed:

1. `evaluate_stmtlist_result_spill_oct14.py`: six C++ source
   variants (ordinary local, volatile local, saved-old local,
   early-continue, named list reference and named result reference);
   three optional branch probability annotations with values
   plain/likely/unlikely; seven historic optimization configurations.
   **1,134 builds**, original-length best 60/119, three authentic
   relocations.
2. `evaluate_stmtlist_preheader_cfg_oct14.py`: explicit early-exit
   guards, `do`/`while`, guarded `for`, loop/return labels, and
   branch prediction, under seven original-era optimizations.
   Some goto variants are rejected by C++03 because they cross
   initialized local scope; those are failures, not matches.
   None improved the 60-byte partial; the best scored
   original-sized candidate had 29/119.

The new candidate is now in `selected_candidates.json` and has
`EXPERIMENTAL` status. It has **not** entered
`exact_candidates.json` or the accepted `vuc/src/` folder.
The user-requested partial is preserved with clear provenance and can
be improved independently without inflating exact coverage.

### Reproduce

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- /tmp/vuc-binutils215/g++-jan25-as215 \
    -O2 -c "$R/probes/translate_stmt_list_partial_oct14.cpp" \
    -o /tmp/vuc-binutils215/stmt-list-partial-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/stmt-list-partial-oct14.o \
    --config "$R/config/translate_stmt_list_partial_oct14_probe.json"
```

## Additional bounded unresolved investigations

`AssembleBinOp(IR::Register const&, IR::Expr_Data*, InstructionList&)`
at `0x0805a3f0` is **193 original bytes** with a previously retained
**162/193** partial and four authenticated relocation targets.

`evaluate_assemble_binop_align_likelihood_oct14.py` compiles
**729** original-era variants covering all 81 combinations of
plain/likely/unlikely type-1/2/3/4 dispatch predicates and nine GCC
alignment/branch-cost modes. The same **162/193** maximum survives,
and no exact dispatch was found. The unsolved bytes involve the
original case-4 block at relative +0x91 versus its candidate pad
before +0x94, and the resulting call-argument/control-flow
difference; register/branch tweaks without original unit-level
evidence are exhausted. The accepted manifest was not touched.

The original `Canonize_Reduce(IR::Fragment_Data*)` at
`0x08050460` reads the kind-2 statement pointer at offset
**+0x18** of the fragment structure, directly proven by original
`mov 0x18(%ebx),...`. Our partial candidate's provisional struct
had incorrectly used **+0x14**. It is now corrected with two
unnamed 32-bit fields before `stmt`, without guessing the
semantic names. Recompilation under the original selected flags
still gives **224 bytes and 78/224 matching original bytes** with
all eleven relocation sites verified. This is a *semantic/ABI
reconstruction correction*, not a new byte-exact or improved byte-count
claim.

## Regression, preservation and next step

Accepted exact function set: unchanged **197/197**,
**14,417/14,417** instruction bytes. The ledger changes from
197 EXACT / 14 EXPERIMENTAL / 87 UNRESOLVED to
**197 EXACT / 15 EXPERIMENTAL / 86 UNRESOLVED**.
Initialized data: **244/244** objects, **71,536/71,536** original
bytes. BSS: **12/12** declarations, 18,437 source allocation bytes.
Host tests: **150/150** (one new guard test), with expected negative
fixture printing an intentional `Candidate byte-exact acceptance:
FAIL` internally.

The 163 original untracked paths and all upstream VCL/VUC binaries
remain preserved, and no new worktree or subagent was used.
No full linked-ELF equality is established.

The next more promising source-level work is authentic compiler
source-unit membership for this translator and recovery of
`IR::Seq_Data` layout/constructor lifetimes. Its early-exit
basic-block arrangement differs significantly, so additional
arbitrary `__builtin_expect` hints are not justified.

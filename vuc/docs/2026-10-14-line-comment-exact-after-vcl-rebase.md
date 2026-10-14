# VUC: byte-exact InstructionList line comment after VCL main rebase

Date: 2026-10-14. Active branch: `vuc`. The session fetched
`origin/main` at `60d0b9a` and replayed 114 VUC commits onto it,
without conflicts. The prior 160 untracked paths were preserved;
none of the original VUC/VCL archives was modified.

## Newly authenticated VUC function

**`InstructionList::AddLineComment(int)`**,
`_ZN15InstructionList14AddLineCommentEi`, original ELF address
`0x08051720`, original size **162 bytes**:

- **162/162 original instruction bytes** match the historical
  January 25, 2004 GCC 3.3.3 prerelease with GNU as 2.15;
- **7/7 relocation sites** resolve against the canonical original ELF;
- **22/22 literal bytes**, at original addresses `0x0807266a`
  (13 bytes), `0x08072677` (8 bytes), `0x08072286` (1 byte),
  are independently authenticated;
- exact flags: `-O2 -frename-registers -falign-jumps=8`.

Accepted independent source:
`vuc/src/InstructionListLineCommentUnit.cpp`.

Accepted manifest, upgraded from a 147/162 experimental candidate:
`vuc/config/instruction_add_line_comment_context_oct13_probe.json`.

The original logic suppresses duplicate line comments when the last
Instruction has kind 3 and the same reserved line number; otherwise it
constructs a 12-byte instruction using an empty `MLGameString`, sets
kind 3 and the line number, and appends it. It includes normal
compiler-generated C++ constructor exception cleanup.

The decisive non-assembly source expression is:

```cpp
if (__builtin_expect(previous->kind == 3, 0) &&
    previous->reserved == line)
    return;
```

This is an ordinary GCC-supported branch-likelihood hint, not a
forced register allocation or synthetic instruction byte. With
`-falign-jumps=8`, it reproduces the original block layout and
exception landing-pad positioning. The original author's exact source
spelling or explicit use of a likelihood hint remains **unproven**.
The source is a verified byte-exact reconstruction, not an authenticated
original C++ listing.

### Reproducible discovery and what did not work

`vuc/probes/evaluate_linecomment_guard_order_oct14.py`
tests twelve genuine duplicate-check formulations (kind first,
line first, nested conditions, inverted/early guards and original
GNU branch-likelihood hints) with eight historical controls each.
The exact result is `NEW_EXACT guard-expect-kind j8`. Earlier normal
source with `-falign-jumps=4` reached **147/162**, verified all
relocations, but differed at the duplicate-check branch and
exception-pad alignment.

`vuc/probes/evaluate_vcl_branch_cost_transfer_oct14.py` checks
`-mbranch-cost=0..4` on the authentic C++ `AddLineComment`,
`TranslateConst` and `MLTempStringData::VFormatString` sources
using both original-era compiler snapshots and multiple GCC alignment
values. These controls do not produce a new exact candidate:
the unmodified line-comment source remains at **147/162** in its
original-length configurations, and `TranslateConst` still lacks a
290-byte candidate. This shows that the historical VCL success
with `-mbranch-cost=0` does not transfer automatically to VUC.

The latest VCL root `docs/matching-notes.md` offers genuine compiler
mechanisms rather than universally applicable flags:

- `prepExtInfo.cpp`: nested guard-arm order corrects seven dead
  GCC 3.2.2 scratch bytes *without* a source-unit boundary change;
- `checkOpt.cpp`: exchanging complete conditional arms and negating
  the predicate corrects dead scratch selection;
- `instStreamLoop.cpp`: historical `-mbranch-cost=0` changes
  GCC IF-conversion and block layout;
- `ProgramUnique.cpp`: real compiler-owner boundaries can change
  postreload scratch-register decisions.

Those facts informed the above VUC source-branch experiments, but
VUC uses GCC **3.3.3**, not the VCL compiler GCC **3.2.2**.

### Source-unit literal-section isolation is mandatory

An initial implementation appended `AddLineComment` directly to
`vuc/src/InstructionList.cpp`. That function matched **162/162**
in isolation, but the *full* historical acceptance suite correctly
rejected the change: its extra one-byte literal changed the
`.rodata` provenance expected by existing accepted
`InstructionList::Add` and `AddB` candidates.

The fix preserves the original `vuc/src/InstructionList.cpp`
**byte-for-byte**, and owns the new exact candidate through a
separate genuine-method-prefix
`vuc/src/InstructionListLineCommentUnit.cpp`. Its five real
`InstructionList` predecessor methods, original 13- and 8-byte
format literals, and final one-byte empty literal are source-level
evidence of the compiler's evaluation context. This split
authenticates one named function while protecting all existing
acceptances; it is **not** evidence that the original application
was split into exactly these files or that the whole binary now
links byte-identically.

### Reproduction

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
  -falign-jumps=8 -c "$R/src/InstructionListLineCommentUnit.cpp" \
  -o /tmp/vuc-binutils215/instruction-linecomment-verified.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/instruction-linecomment-verified.o \
  --config "$R/config/instruction_add_line_comment_context_oct13_probe.json"
```

Compiler-generated intermediate source, object and diagnostic data
remain under `/tmp/vuc-binutils215` inside the existing Lima VM.

## Coverage and limits

The accepted set increases by **one function**, from 196 to
**197 EXACT / 14 EXPERIMENTAL / 87 UNRESOLVED** of the 298 named
functions, totaling **14,417 accepted instruction bytes**.
This is *per-function* byte identity, **not** a complete
linked-ELF match. Keep the initialized-data and BSS regression
gates separate and mandatory.

After isolating the new source unit, the complete independent gates
pass: **197/197 matched functions**, **14,417/14,417 instruction bytes**,
**244/244 initialized-data objects**, **71,536/71,536 initialized-data
bytes**, **12/12 BSS declarations** (18,437 source allocation bytes),
and **149/149 host tests**.

Next candidate: the unresolved Instruction arena static destructor
`__tcf_1` remains 45/46 in prior authenticated controls, but
the newer GCC branch-guard evaluator is kept experimental until all
configurations can be verified. The preceding session's
`TranslateConst` / `Value::ToString` / historical STL reserve
partial reconstructions are preserved and not promoted.

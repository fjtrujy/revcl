# Lessons from VCL main for byte-exact VUC reconstruction

Reviewed: 2026-10-14. Repository: `revcl`, branch `vuc`.
Latest reviewed VCL `main` baseline: **`1aadee5`**.
Previous VUC `vuc` head: `a35c68e`; rebase replayed **107**
commits, producing `7858480` before any new VUC documentation.

This is an *evidence index and experiment plan*, not a claimed new
matching function. It distills `docs/decompilation.md`,
`docs/workflow.md`, `docs/matching-notes.md`, all 19
`config/matching-evidence/*.md` cases and the 13 newly fetched
VCL commits from `dae800f` through `1aadee5`.

## 1. Different targets and evidence thresholds

| Property | Original VCL | Original VUC |
| --- | --- | --- |
| Original compiler evidence | GCC **3.2.2** Red Hat 20030222 | GCC **3.3.3** prerelease dates 20031206 / 20040125 |
| Historical assembler | RH9 binutils 2.13.90 family | Recovered GNU as **2.15**, compatible but **not proven original** |
| Binary provenance | Stripped executable; inferred function symbols | Original `.symtab`, `.strtab`, 22 `.cpp` source-unit records, 79 local text symbols |
| Application DWARF | VCL analysis tools/IDA work | **No VUC C++ application DWARF units**; four DWARF CUs are glibc startup files |
| Matcher maturity | Functions, literals, unwind and linked-image score | Strict accepted function, initialized-data and BSS gates; **no independently validated whole-ELF score yet** |
| VUC working inventory | Not applicable | **195 exact / 15 experimental / 88 unresolved** out of 298 named functions |

The VCL findings are **hypotheses to test against VUC's original ELF and
its two GCC 3.3.3 snapshots**, not instructions to adopt GCC 3.2.2
switches, register orders, linker scripts, or code shapes without proof.

VCL's main branch documents *explicit user-authorized* speculative
translation-unit partitions and compiler controls that are not proven
historical boundaries or original author choices. Such permissions
belong to that particular workflow. **They do not automatically
authorize speculative VUC partitions or flags.** Follow
`vuc/AGENTS.md`: plausible historical source, strict full instruction
and relocation matching for acceptance, partial work only in
`vuc/probes/`, never force registers or padding with assembly.

## 2. First decide *which compiler phenomenon* explains the mismatch

Classify differences before changing source; one class often looks like
another in an instruction-aligned diff:

1. **Wrong semantics, control-flow graph or types**: early/out-of-line
   returns, signed/unsigned predicates, byte vs int bools, incomplete
   destructor cleanup, incorrect inline getter, duplicated source
   field load, missing true/false join, wrong switch-case grouping, or
   genuine missing parameter. Recover the correct C++ logic first.
2. **Early optimization, induction variables and live allocation**:
   trace initial RTL, loop, GCSE, local/global allocation
   (`lreg`/`greg`) and postreload. Changing a source-level local,
   its scope, member vs local usage, inline wrapper or paired C++ source
   expression may be necessary. **Peephole2 scratch rotation cannot
   repair an early incorrect live spill or IV.**
3. **Dead scratch registers, postreload and peephole2**:
   only after proving a saved/discarded word is *dead*, inspect
   `flow2 -> peephole2 -> rnreg -> sched2`. GCC 3.2.2's
   `peep2_find_free_register` uses a static per-process search
   offset, carried across processed functions in a translation unit.
   This can affect both scratch pops/pushes **and useful rnreg chains**.
   Its corresponding behavior must be verified experimentally under
   VUC's GCC **3.3.3**, not inferred from 3.2.2 implementation code.
4. **Basic-block probabilities and assembler label alignment**:
   `bbro` frequencies, condition polarity, nested inline predicates,
   shared labels, source-case order and a genuine branch hint can
   add/remove a compiler-requested `.p2align` while preserving useful
   instructions. One changed NOP shifts later relative branches and
   makes apparently unrelated relocation references fail. Compare
   `-S` and `-da` before changing relocation maps.
5. **Translation-unit emission order and inline ownership**:
   mapped address order is **not necessarily GCC pass-processing
   order**. Deferred `#pragma interface/implementation` members,
   C1/C2 and D1/D2 clones, weak/linkonce copies, and template
   emission often move across ordinary definitions. A shared
   `.eh_frame` CIE group is not proof of a shared `cc1plus`
   scratch state. Use actual ELF source-unit symbols and emission
   evidence where available.
6. **Real initializer/exception/unwind/linked-data ownership**:
   string contents, NUL and zero alignment tails, BSS, RTTI/vtables,
   LSDA landing ranges, GCC `.comment` tags, assembler multi-byte
   NOPs vs linker `0x90` fill and whole-linked offsets are separate
   from relocation-resolved function code. A newly exact function
   can still make a whole ELF less faithful.

For every promising hypothesis, save the original disassembly, exact
source/flags, independent original-ELF relocation validation, candidate
length, actual RTL stage of first operand/liveness divergence,
negative controls, and any changes to other previously exact bodies
or literal emissions.

## 3. Specific techniques supported by the new main commits

| Main evidence | Compiler/source explanation | Transfer to VUC |
| --- | --- | --- |
| `program-unique-split.md` | A real target/successor fresh owner changes **two dead cleanup pops** (699/701 to 701/701); full and isolated candidate RTL diverge at peephole2 | Compare genuine predecessor emission/context for the one-byte `Instruction` arena destructor, **without changing the destructor body** |
| `inststream-loop-reconstruction.md` | Correct guarded arithmetic plus scoped `-mbranch-cost=0` suppresses if-conversion, keeps false-first AL path (85/97 to 97/97); the true code/data owner must preserve string alignment | Target byte-selector/return-layout problems only after measuring `ce` and `bbro`; do **not** copy branch-cost flag or assert original VUC build used it |
| `collapse-reconstruction.md` | Genuine predicate `__builtin_expect` changes branch frequency and removes just one internal `.p2align`, yielding 775/775 with identical useful instructions | Highly relevant to `Value::operator+()`'s asymmetric scalar/vector alignment; prior generic `expect` sweeps failed, so examine a *specific original predicate/edge* before any narrower experiment |
| `aa-fixup-split.md` | Splitting before the target fixes peephole/rnreg snapshots but **not** an early constructed address or extra result join: 946/1180 to 955/1180, still inexact | Mark scratch-only improvements separately from structural recovery; never interpret a small score gain as proof the whole source is right |
| `epipro-reconstruction.md` | A genuine loop-condition-carried receiver plus **direct count parameter** prevents full-address induction-variable formation; real unchanged constructor predecessor then fixes 24 remaining bytes (2107 to 2573 to 2597 exact) | For `yy_get_next_buffer()` investigate pointer/size induction and aliasing **before** register/align flags; do not generalize the exact same helper signature |
| `fdep-register-users-reconstruction.md` | The earliest useful genuine prefix fixes two dead pops (377/378 to 378/378), but shorter prefix cuts discard actual 32-aligned diagnostic zeros | Preserve all referenced originals including complete string sections when testing VUC real predecessor suites |
| `buildfdeplist-reconstruction.md` | A real head carrier through a second-empty edge eliminates redundant PRE; a non-dereferenced 0/1 continuation token changes live register allocation. Selective prediction restores inner-loop NOP (to 1713 exact) | Investigate true pointer lifetime/SSA phi nodes in `Value` and Flex; changing locals is productive only with a semantics argument and original RTL corroboration |
| `add-thruput-regrouping.md` | One unchanged real predecessor repairs six dead scratches (498/589 to 504/589), yet the original **live timestamp stack home remains absent** | Targeted compiler-state tests are worthwhile; do not claim exactness from repaired pushes while saved live-value homes remain wrong |
| `prune.cpp` notes in `docs/matching-notes.md` | An actual dead pointer reset `lookup = 0` can defeat GCSE's source-copy forwarding after ADDRESSOF promotion while emitting no instruction; enabled 871/871 | Test only where original stores/joins establish that real control flow; never add meaningless dead stores purely as machine-code padding |
| `emu.cpp` notes | Moving a real shared `fail:` label inside a switch changes **peephole2 traversal order** and recovers 573/573 without file split or compiler switch | Consider legitimate shared error/EOF labels in Flex/AST dispatch; label location may matter even if final optimized CFG appears unchanged |
| `fiddleBDL.cpp` notes | Nested RAII lifetime scopes with natural success fallthrough change destructor emission and register renaming (1018 to 1031/1031) | Verify `.gcc_except_table` call-site ranges for STL/AST/Value functions before changing cleanup paths |
| `regweb-no-weak.md` (earlier) | `-fno-weak` and `-ffunction-sections` both reproduce function bodies/relative offsets; **assembler vs linker gap fill** identifies the former more strongly | Validate original inter-function alignment bytes before inferring weak linkage or a source owner from body exactness |
| `parser-buffer-reconstruction.md` (earlier) | Scoped `-fno-gcse` fixes real PRE/stack webs, but a complete unit regresses neighbours; requires genuine diagnostics + mergeable constant pool ownership | Never apply VCL's flag globally; test original VUC flags, emitted `.rodata` literals, and exact neighbours independently |

All these experiments explicitly separate an **observed byte match**
from proof that a source construct, ownership boundary or nondefault
compiler flag was present in the original author's source.

## 4. Earlier reusable evidence, including relevant failed hypotheses

These are not just successes; the rejected alternatives explain why
a seemingly easy local change often misleads:

| Evidence | Main practical distinction |
| --- | --- |
| `cgreadfile-split.md` | Original GCC 3.2.2 `recog.c` static scratch-search mechanism and genuine fresh `cc1plus` boundaries; don't blindly alter exact predecessor bodies |
| `mnode-emission-order.md` | Real definition placement can recover a one-byte dead pad; all unrelated neighbours must remain exact |
| `xun-lifetime-split.md` | `#pragma interface/implementation` can emit real D1/D2/C1/C2 in a separate owner even if no normal function exists; D2 may become exact while D1 remains 49/50 |
| `line-owner-split.md` | One genuine Line owner recovers D1, setter and clear scratches together, while preserving actual destructor/data owners |
| `visitcache-owner-split.md` | Separating genuine deferred VisitCache from subCache fixes three pads, but both owners must move to avoid regressions |
| `dumppathkey-deferred-owner.md` | Free inline function emitted *after* real deferred members can inherit needed peephole2 state; historical ownership is unproven and even whole-file score can drop |
| `dyncalcstats-split.md` | Fix the loop's indexed spelling first; separate original-source context then fixes useful register chains and dead cleanups, with distinct native/linked residuals |
| `loc-tag-reconstruction.md` | Switch-case **source order** changes live pseudo allocation; genuine validity-helper predecessor then corrects peephole2/rnreg |
| `link-oparts-reconstruction.md` | Named embedded-member receiver determines a *live* LEA/push chain; only after that change does the genuine source-unit prefix make it exact |
| `program-unique-split.md` | Isolated exact result alone is not sufficient; keep true original literal-tail/data coverage and unaffected predecessor functions |
| `aa-fixup-split.md` | Source-context-only register improvement can leave an entirely wrong initial RTL and local-slot topology |

Source idioms in `docs/matching-notes.md` are best treated as a
catalogue of compiler experiments, not an answer key:
pointer-local vs direct field access; `bool` vs `int` vs
`unsigned char` and signed shorts; case-label order; two-level
inline predicates; explicit shared `goto` epilogues vs per-case
returns; inline wrappers flushing pending call argument cleanup;
non-null/false-first source structure; loop index `<` vs `!=`;
array-index vs pointer-cursor induction; and declarations/order
affecting stack-slot placement. **Each trick can help one function
and break another.** Use original byte evidence to select it.

## 5. What the VUC checkout has already proved

Useful *positive* examples in VUC, compiled against the historical
ELF with the GCC 3.3.3 prerelease and GNU as 2.15:

- `Value::operator*()` is **222/222** and
  `Value::operator-()` **221/221** with the real `+`, `-`, `*`
  predecessor source context. This confirms translation-unit
  compiler state matters in VUC itself, not just VCL.
- `Value::TypeConvert()` is **220/220**, using real nested
  `switch` branches and the original early/late return topology.
- `AlignString()` is **145/145** and its eight original relocations
  resolve from an isolated genuine function: the five-method
  `AsmOutput` context was only 143/145. This is evidence
  **against assuming that a bigger unit is always better**.
- Four recent Flex methods are exact:
  `yy_try_NUL_trans` **161/161**,
  `yy_get_previous_state` **205/205**,
  `yyinput` **170/170**, and
  `yyunput` **167/167**. Signed 16-bit transition tables,
  source-level cached byte lifetime, original switch fallthrough
  and genuine buffer-shifting loops mattered more than flags.

Useful negative examples in VUC, to **avoid repeating unchanged**:

- `Instruction` arena `__tcf_1`: **45/46**,
  original dead `pop ecx` vs compiled dead `pop ebx`.
  Both GCC 3.3.3 snapshots select EBX; 25 real method orders,
  deferred-header owners and basic flags did not recover ECX.
  `flow2/peephole2/rnreg/sched2` diagnostic evidence is in
  `vuc/docs/2026-10-13-rtl-printer-ir-negative-batch.md`.
- `Print(ASTFunCall*)`: **96/98**, register choices in the
  callee pointer load. Multiple genuine printer source orders and
  deferred-header controls did not improve it.
- `Value::operator+()`: **172/219** original-length best;
  original vector branch starts `+0xa7`, scalar branch
  `+0xd0`. Natural controls align both or neither. Eighty
  alignments and prior generic branch-probability forms failed.
- `Value::operator/()`: **178/235** best authentic scalar/
  x87 context, not exact. Investigate reciprocal vs in-place
  float evaluation and actual register/stack lifetimes.
- `yy_get_next_buffer()`: **430/550** with original
  `realloc` and scanner diagnostic relocations. Tested loop
  spellings, genuine predecessor methods, source pointer/owner
  forms and global alignment/optimizer sweeps did not improve it.
  Experimental literal padding maps to original zero bytes but
  **does not establish original translation-unit literal layout**.
- `GetNextWord()`: the original is **135** bytes; the natural
  nested byte-compare source is **127** and not eligible for
  original-length acceptance. Local `register`, char, increment
  order and original-sized variants remain negative.
- `Translate(ASTNodeList<ASTStmt*>&)`: original **119**
  bytes, but tested `IR::Seq_Data` layouts/cursor forms have
  not reproduced that length.
- `MLTempString::Set()` **111/117** in isolated source; more
  complete source units can worsen it. Check original literal
  ownership and constructor emission before adding neighbours.

## 6. An evidence-first VUC work sequence

1. **Collect the target:** `vuc/scripts/decomp.py asm <addr>`,
   the exact original length, symbol prototype, section/relocation
   map, original data/literal bytes, call targets and source-unit
   clues (`vuc/scripts/source_units.py`, original .symtab).
   VUC application DWARF cannot supply missing C++ signatures.
2. **Classify the first real mismatch**: compare the original and
   emitted object instruction-by-instruction, *after* resolving
   original relocations. Fix absent calls, wrong field offsets,
   widths/signs, x87 operation order, constructor/LSDA lifetime
   or incorrect block/loop shape before targeting NOPs.
3. **Use genuine minimal source controls:** method isolated vs
   original-ordered real-prefix/suffix; case-arm/label order;
   pointer/index spelling; signedness and real helper inlining.
   Record an exact-function regression guard for every
   previously exact neighbour.
4. **Only if the difference is scratch/spacing**, inspect
   diagnostic `-da` and `-S` objects built by
   `/tmp/vuc-binutils215/g++-jan25-as215` inside
   `revcl-oracle`. Compare `greg`, `flow2`,
   `peephole2`, `rnreg`, `sched2` and `bbro`
   as appropriate. Use real source context, not dummy methods
   or an artificial dead-pop destination.
5. **Distinguish provenance**: confirm both the candidate
   symbol's *full* original instruction length and byte equality,
   *all* relocation operands/targets, all emitted initialized
   literals/RTTI data and BSS. Original literals need their
   actual original ELF contents; matched zero padding at an
   unrelated address is not historical source-unit proof.
6. **Accept conservatively:** only full accepted matches go in
   `vuc/src/` and `vuc/config/exact_candidates.json`.
   Original-length partials remain under `vuc/probes/` and
   selected diagnostic manifests. A near-exact function does
   not become exact by ignoring padding or unmatched bytes.
7. **Full regression** (separate compiler/data/BSS gates):
   ```sh
   python3 -m unittest discover -s vuc/tests -q
   python3 vuc/scripts/coverage.py --write
   R=/path/to/revcl/vuc
   C=/tmp/vuc-binutils215/g++-jan25-as215
   limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" --compiler "$C" --out /tmp/vuc-exact-check
   limactl shell revcl-oracle -- python3 "$R/scripts/match_data_candidates.py" --compiler "$C" --out /tmp/vuc-data-check
   limactl shell revcl-oracle -- python3 "$R/scripts/match_bss_candidates.py" --compiler "$C" --out /tmp/vuc-bss-check
   ```
   These checks still do **not** prove complete linked ELF
   identity. Do not call VCL's 482,736-byte score a VUC score.
8. **Preserve and report:** no worktree, subagent, source archive,
   root VCL edit, fabricated code, lost exact symbol, or
   unexpected untracked-file deletion. Commit only VUC work.
   Each experiment report must explain the original hypothesis,
   the failure controls and the actual verified gain (including
   **zero** if that is the outcome).

### Next practical priority

The best immediate **new-evidence** experiment is to isolate
`Value::operator+()`'s exact vector/scalar label-frequency
asymmetry with the real original switch/return graph and
`-da` branch-frequency evidence. The old broad
`__builtin_expect` forms are already negative; a useful
experiment must identify a particular original control-flow
edge with different predicted frequency, not repeat the
old alignment sweep.

For a likely **new exact-function** attempt, the 45/46
`Instruction` cleanup is small, but only investigate it
after locating a genuine source-unit emission/processing
predecessor with a specific peephole2-state prediction.
The VCL 3.2.2 observations do not prove that such a
predecessor exists in VUC 3.3.3.

A more structural batch should use `yy_get_next_buffer()`
and reconstruct its buffer-growth/ownership CFG before
trying further global alignment or register-flag combinations.

## 7. Primary reference index

All files below belong to **VCL main** and should remain read-only
during VUC reconstruction:

- Compiler/toolchain and scoring: `docs/decompilation.md`
- Engineering workflow, evidence gates and pitfalls: `docs/workflow.md`
- Broad source-to-assembly idioms and failed forms:
  `docs/matching-notes.md`
- Detailed original-compiler reproductions:
  `config/matching-evidence/{cgreadfile-split,mnode-emission-order,
  xun-lifetime-split,line-owner-split,loc-tag-reconstruction,
  link-oparts-reconstruction,dyncalcstats-split,visitcache-owner-split,
  dumppathkey-deferred-owner,regweb-no-weak,parser-buffer-reconstruction,
  add-thruput-regrouping,buildfdeplist-reconstruction,
  fdep-register-users-reconstruction,epipro-reconstruction,
  aa-fixup-split,collapse-reconstruction,inststream-loop-reconstruction,
  program-unique-split}.md`
- Individual source and rejected-probe measurements:
  the adjacent `*-measurements.json`, `*-compiler.txt` and
  `*-repro.py` files.
- VUC-specific prior negative controls: the recent dated
  `vuc/docs/2026-10-13-*.md` and
  `vuc/docs/2026-10-14-value-plus-word-scanner-controls.md`.

The strongest lesson is **do not confuse matched emitted code,
compiler-state correlation, and historically authenticated
source or complete ELF identity**. Use all three as
separate validation milestones.

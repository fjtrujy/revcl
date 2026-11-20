# VUC 2004.1: VCL technique transfer, historical GCC controls (2026-11-20)

This is an additional **VUC-only** reconstruction round after local commit
`c855880` on isolated branch `vuc` at
`/local-home/Projects/.revcl-vuc-rebase-20261120`.
The original oracle is the immutable ZIP-backed
`X86linux_vuc2004_1/vuc` ELF32-i386 executable, not VCL. The canonical
repository's checked-out `main` and other agent's files were not touched.
No original executable/source archive was modified; no push.

## Why the VCL project mattered

Read VCL's `docs/matching-notes.md`, `docs/research/buildCandidateList.md`,
`config/matching-evidence/program-unique-split.md`, and the existing
`vuc/docs/2026-10-14-vcl-main-byte-matching-playbook.md`. The VCL results
identify several **distinct, testable** compiler mechanisms:

1. Genuine source-unit emission order changes the cross-function
   `peep2_find_free_register` scratch cursor, sometimes fixing a one-byte
   *dead* stack pop without changing the function body (e.g.
   VCL `Program::doUnique`, `buildCandidateList`).
2. Real branch probabilities alter GCC's block-layout alignment, sometimes
   adding/removing only compiler-generated `nop`/`lea` padding.
3. Real pointer constness, inline accessors and field/type declaration
   identities can modify global allocation; superficial aliases are often
   canonicalized before the relevant RTL pass.
4. A true predecessor/template source-unit context must preserve exact
   siblings, authentic data/relocations and compiler flags. A function
   exactness score is **not** a linked whole-binary score.

VCL's recovered compiler is GCC **3.2.2** (Red Hat); VUC's usable,
still-not-authenticated-original compilers are GCC **3.3.3 prerelease
2004-01-25** and **2003-12-06**, each with **GNU as 2.15**. VCL
mechanisms are hypotheses to check, not tools to substitute or proof
of an original VUC source order.

## 1. STL normal-iterator uninitialized copy (0x0806a840, 149 bytes)

The historically natural `SymbolTableUnit.cpp` source emits **145
bytes**, with the six original exception/constructor call relocations
and **92 consecutive identical original-offset bytes**. The source
assembler contains an `.align 16` before the normal return at
relative `+0x50`, but **no** alignment before the exception entry
at `+0x5c`. The original has a four-byte assembler-generated
alignment at `+0x5c..+0x5f` and catch entry at `+0x60`.
The 145-byte source **cannot be scored as an authenticated
original-length partial**.

Reproducers:

- `vuc/probes/evaluate_scope_likelihood_20261120.py`:
  5 genuine loop-prediction C++03 forms x 2 compiler snapshots =
  **10 builds**. The original historical condition is 145 bytes;
  predicting the loop as likely emits 136 bytes. Predicting it
  *unlikely* emits the original **149-byte length**, but only
  **61/149 matching bytes** (all six relocations still valid), with
  most of the original 92-byte prefix lost. Hence matching the
  total size does not solve compiler block ordering.
- `vuc/probes/evaluate_scope_cleanup_likelihood_20261120.py`:
  5 genuine exception-cleanup loop predictions x 2 snapshots =
  **10 builds**. Every build emits **145 bytes**. All four
  independently exact SymbolTable siblings remain exact.

Both experiments use **SHA-pinned copies** of actual historical
`bits/stl_uninitialized.h` and `bits/stl_construct.h` exclusively
inside the guest's `/tmp`, without changing installed headers or
production inputs. The latter controls reveal that simply changing
the expected frequency of the cleanup loop does not add the pad.

Guest evidence: `/tmp/vuc-gcc33-eh-20261120/`,
`/tmp/vuc-scope-likelihood-20261120/`,
`/tmp/vuc-scope-cleanup-likelihood-20261120/`.

## 2. Value::operator+ (0x0806e9b0, 219 bytes)

The selected real arithmetic source remains a fully authenticated
**203/219**, one TypeConvert relocation, with mismatches
`+169,+172,+175,+194..+206`. Its vector lane lexical
order is `3,1,2,0`; the natural ascending source compiles to
**216**, not 219 bytes. The original scalar x87 case starts at
relative `+0xd0`, while the ascending version starts at
`+0xcd`: the *three-byte alignment* is missing despite genuine
ascending arithmetic instructions.

Reproducers:

- `vuc/probes/evaluate_value_plus_likelihood_20261120.py`:
  12 C++ switch/TypeConvert predictions x 2 GCC snapshots =
  **24 builds**. The selected candidate remains 203/219;
  all natural-source prediction candidates emit 201–216 bytes,
  never the authenticated 219-byte target.
- `vuc/probes/evaluate_value_case_guard_20261120.py`:
  10 source-level scalar/vector case-local actual-predicate guards
  x 2 snapshots = **20 builds**. Every candidate still emits
  **216 bytes** and preserves its TypeConvert relocation. These
  guards are optimized out before the needed alignment decision.
- `vuc/probes/evaluate_value_tag_type_20261120.py`:
  6 plausible signed/unsigned/enum/const C++03 tag types,
  both lane orders x 2 snapshots = **24 builds**.
  Signed `int`, signed `long`, and a 32-bit enum preserve
  Value+ **203/219** and both independently exact
  `operator-` **221/221** and `operator*` **222/222**.
  Unsigned forms lose one byte in all three functions;
  a const tag emits a wrong-sized 125-byte body and cannot
  preserve accepted siblings. There is no signedness/tag-type
  explanation for the three-byte scalar alignment.
- `vuc/probes/evaluate_value_division_successor_20261120.py`:
  6 genuine Value source-unit contexts (selected or ascending
  independent vector lanes, with original-order division before or
  after multiplication) x 2 compilers = **12 builds**.
  Adding the real source-level `Value::operator/` successor
  (which remains **228** bytes against **235** original) does
  not move any Value+ case labels: the selected source stays
  **203/219** and the ascending source **216** generated bytes.
  The source unit preserves both exact `operator-` **221/221**
  and `operator*` **222/222** across all six contexts.
  This rules out simple successor co-instantiation/emission order,
  not an unidentified original definition in that owner.

Generated objects and JSON ledgers are under
`/tmp/vuc-value-plus-likelihood-20261120/`,
`/tmp/vuc-value-case-guard-20261120/` and
`/tmp/vuc-value-tag-type-20261120/`, plus
`/tmp/vuc-value-division-successor-20261120/`.

## 3. ASTFunCall printer (0x0805b770, 98 bytes)

`vuc/probes/evaluate_printer_const_owner_20261120.py`
tests **6** real pointer/member constness or restrict contracts with
both compilers (**12** builds), transferring VCL's confirmed
`Operand *const` allocation effect. Every original-sized object
is **96/98**, with exactly two differing bytes `+23,+29`
(the original selects ECX for the callee load; GCC chooses EAX).
All seven authentic relocations and nine literal bytes survive.
This establishes that the tested parameter and member declarations
do not change GCC3.3's hard-register assignment. Source aliases
already ruled out in previous VUC reports were not repeated.
Guest ledger: `/tmp/vuc-printer-const-owner-20261120/`.

## 4. Instruction arena __tcf_1 (0x08051980, 46 bytes)

This is the closest VCL peephole state analogy. The original function
has a dead `pop ecx` at relative `+42`; the faithful C++03 arena
cleanup emits a **46-byte** function with all three authentic
relocations but `pop ebx` (strict **45/46**).

New reproducible source-owner studies:

- `evaluate_instruction_template_order_20261120.py`:
  7 placements of the **genuine**
  `std::vector<Instruction*>::_M_insert_aux` explicit template
  instantiation (rather than a fake predecessor) x 2 GCC snapshots
  = **14 builds**. Placing it **before** the real
  `InstructionList::Add` changes the cleanup scratch to
  **EAX** from **EBX**, while retaining exactly **45/46**.
  This is real evidence that the source-unit template emission
  order reaches the dead scratch choice. The concurrently emitted
  vector helper in that minimal owner is 276 bytes, versus
  original 265, so this source unit is not accepted.
- `evaluate_instruction_owner_composed_20261120.py`:
  3 genuine reconstructed InstructionList source sequences x 5
  template placement contexts x 2 historical GCC snapshots
  = **30 builds**. The same source-order effect
  (early instantiation makes `pop eax`) appears in all
  three method groups; no candidate chooses original ECX.
  The cleanup stays 45/46 with all three relocations.
- `evaluate_instruction_exact_vector_owner_20261120.py`:
  7 genuine complete source-owner/instantiation positions x
  2 historically meaningful flag sets x 2 compiler snapshots
  = **28 builds**. The owner derives directly from the already
  exact `InstructionListLineCommentUnit.cpp`, brings in the
  original arena source and preserves proper `<iostream>`
  static-object ownership. With `-fuse-cxa-atexit`,
  `-falign-jumps=1`, this combined object reproduces
  the authentic **265/265** vector insertion (5 relocations)
  **and** **45/46** cleanup (3 relocations), together.
  Placing the real vector instantiation before Add changes
  its cleanup scratch from EBX to EAX while retaining the exact
  vector. No ECX. Without `-fuse-cxa-atexit`, the cleanup is
  emitted as `__tcf_0` instead of the original `__tcf_1`;
  the vector commonly scores **233/265** in those controls;
  the `vector-before-add` exception retains **265/265**.
  None is a valid new acceptance for the differently named cleanup.

The accepted 265-byte vector body already exists, so recreating
it in a different scratch unit **does not add another exact
function**. The narrower next genuine discriminator for the
one-byte pop is the weak vector/iostream cleanup processing order
and GCC `peephole2`'s *incoming and outgoing* TU-wide scratch
cursor; do not repeat arbitrary register aliases or add dummy
source functions.

Guest ledgers: `/tmp/vuc-instruction-template-order-20261120/`,
`/tmp/vuc-instruction-owner-composed-20261120/`,
`/tmp/vuc-instruction-exact-vector-owner-20261120/`,
`/tmp/vuc-instruction-predecessor-20261120/`.

## 5. Compiler provenance control

`vuc/probes/evaluate_compiler_version_20261120.py` compares
**5 real VUC targets x 2 compilers = 10 builds**, with
January 2004 GCC 3.3.3 vs a **noncanonical** Debian GCC 3.3.5
binary, both using historical as 2.15. The later compiler
still produces the **145-byte** scope helper and the **45/46**
cleanup (same +42 pop). It changes Value+ selected from 219 to
203 bytes and natural from 216 to 200, rendering both ineligible.
The printer's merged literal-section relocation is no longer
mapped in the later compiler object, so no strict printer score
can be inferred. Debian GCC 3.3.5 is **not** a substitute for
the original 2004 Debian 0pre3 compiler/patches, which are
still unauthenticated. The control weakens the hypothesis that
a later generic GCC bug fix alone produces the missing bytes.

Guest ledger: `/tmp/vuc-compiler-version-control-20261120/`.

## 6. Original Instruction::instrData global-owner position

An additional genuine-ownership discriminator followed the exact-vector
source-unit experiment. The original 31-byte startup function at
`0x080519b0` is named
`_GLOBAL__I__ZN11Instruction9instrDataE`. The initial combined owner
instead named its generated constructor after
`InstructionList::Add`; this is direct evidence that the reconstructed
first strong definition is in the wrong position. Reproducer:
`vuc/probes/evaluate_instruction_global_owner_20261120.py`.

**Nine real C++03 positions/orderings x two GCC snapshots (18
builds)** move the *unchanged, genuine*
`char *Instruction::instrData;` definition before or after real
methods and, in two controls, compose it with the authentic STL vector
instantiation. When it is placed before the first InstructionList
method, the **original constructor symbol name is recovered**, with
**29/31** original-length startup bytes (differences at
`+23,+24`); it still does *not* qualify as a new exact startup
candidate, and the startup is already exact in the separate
original-manifest source unit. Every run preserves the original
`std::vector<Instruction*>` **265/265** and iostream `__tcf_0`
**16/16**. The arena cleanup remains **45/46**, with its final
dead scratch either EBX, or EAX when the genuine vector instantiation
is explicitly emitted before Add. Neither composing the two causal
changes nor changing their order yielded original ECX.

This recovers a meaningful original *source-symbol ownership property*
without treating a 29/31 new symbol as exact or sacrificing existing
exact functions. Full per-case scores, compiled originals and candidate
sources live in `/tmp/vuc-instruction-global-owner-20261120/`.
Further investigation should change the *actual preceding* compiler
state, not reorder the already disqualified
`instrData` declaration variants again.

## Acceptance, preservation and next step

These **twelve reproducible probe campaigns comprise 212 compiler
builds** (including baselines and negative controls). They do
**not** produce an admissible new exact function or an improved
authenticated original-length partial. No prototype, probe header,
experimental compiler, accepted `vuc/src/` body, original archive,
manifest or VCL root file was changed; no one-byte overlap claim
was promoted without all original-length/relocation checks.

The strict baseline remains **207 EXACT / 37 EXPERIMENTAL /
54 UNRESOLVED**. Existing accepted function score:
**207/207, 16,723/16,723** verified code bytes.
Existing authenticated partial score:
**36/37, 7,183/11,134** original-offset matched bytes,
with `SymbolTable::Enter` wrong-sized and unverified.
Named initialized-data gate is separate:
**244/244, 71,536/71,536** original named data bytes.
BSS is separate: **12/12**, 18,437 allocated bytes
with zero file-backed BSS bytes. The complete linked VUC ELF
has **NO verified whole-binary score**.

Next evidence-backed investigation should inspect GCC 3.3's actual
TU-local peephole2 scratch-search trajectory around the real
Instruction vector and both cleanup callbacks, *without*
making exact predecessors inexact. For the 149-byte Scope
helper, inspect the original EH catch-edge block prediction
versus the generated `.32.bbro` after the loop branch hints;
only a genuine source-level change that preserves the first
92 original bytes is a meaningful candidate. For Value+,
source-level branch effects must reproduce the extra
three-byte natural label alignment *while preserving* the
full 219-byte function and exact arithmetic siblings.

### Independent regression verification (completed after experiments)

All complete production and test checks were rerun from the unchanged
VUC source with the historical January compiler wrapper and the existing
VM's VUC-only snapshot, **not** with modified scratch headers or the
noncanonical GCC 3.3.5:

| Gate | Verified result |
| --- | --- |
| `match_candidates.py` | **PASS: 207/207 exact, 16,723/16,723 code bytes** |
| `match_partial_checkpoints.py` | **PASS: 36/37 authenticated lengths, 7,183/11,134 original-offset bytes, zero regressions; Enter unverified** |
| `match_data_candidates.py` | **PASS: 244/244 named data objects, 71,536/71,536 bytes** |
| `match_bss_candidates.py` | **PASS: 12/12 declarations, 18,437 allocated bytes, zero file-backed BSS bytes** |
| Host `unittest discover -s vuc/tests -q` | **PASS: 205 tests** |

The host test deliberately exercises a non-exact candidate, whose
subprocess prints `Candidate byte-exact acceptance: FAIL` as an
expected **negative-test fixture**, while the full unittest exits 0.
All independently generated gates continue to state that a native
whole-binary matching score is **UNAVAILABLE**.

```sh
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_candidates.py --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-vcl-transfer-exact-20261120
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_partial_checkpoints.py --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-vcl-transfer-partial-20261120
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_data_candidates.py --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-vcl-transfer-data-20261120
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_bss_candidates.py --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-vcl-transfer-bss-20261120
/path/to/revcl/.venv/bin/python -m unittest discover -s vuc/tests -q
```

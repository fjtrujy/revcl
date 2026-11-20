# VUC 2004.1 — upstream rebase and historical compiler regression checkpoint

**Date:** 2026-11-20. **Project:** VUC only (`vuc/`), not the VCL matching project at repository root.

## Safe multi-agent workspace

- Canonical repository: `/path/to/revcl`.
- Other agent currently works in the canonical repository's checked-out `main`; leave it and its existing untracked files untouched.
- Isolated full **clone** (not `git worktree`): `/local-home/Projects/.revcl-vuc-rebase-20261120` on local branch `vuc`.
- Source `vuc` branch and `origin/vuc` before rebase: `2109c25ca20172580ddb0af67c9d36074476f01a`.
- Rebased onto latest published `origin/main` at `47e0c6fb48bf65d6025058de80e52ae6efaca42d`.
- All 191 commits replayed cleanly in the isolated clone. Immediate rebased source HEAD before this report: `c8f585a4a585e0baa1e6dd7c38a0b2d72d18016f`.
- The exact `vuc/` Git tree remained unchanged (`cd9bca4d2695822366dc92f40a64018904a2d86f`). The canonical repository's local `refs/heads/vuc` was updated to the rebased SHA by a guarded fetch and `git update-ref` without checking out any branch in the active `main` workspace. No push.
- The parent repo's checked-out `main` and preexisting untracked paths were not changed. `origin/vuc` still refers to the old pre-rebase history. Never force-push without explicit user approval.

## Verification against the ORIGINAL VUC 2004.1 ELF32-i386 executable

The canonical target remains the ZIP member in `archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip` (314,338-byte ELF32, SHA-256 recorded in `vuc/README.md`). Preserve its bytes.

The independent target inventory passes, showing **298 named text symbols**. Host test run with the canonical repo's Python environment against the isolated clone: **205/205 tests pass**.

The Lima VM `revcl-oracle` was restarted successfully. Its persistent historical toolchain cache was present, and `vuc/scripts/restore_cached_toolchain.sh` restored the `/tmp/vuc-binutils215/g++-jan25-as215` wrapper. It identifies as **GCC 3.3.3 20040125 prerelease** with a period-compatible **GNU as 2.15**. These are recovered candidate tools; the precise original Debian binaries are not authenticated.

The isolated clone was not mounted inside the VM, so a VUC-only test snapshot was copied into `/tmp/revcl-vuc-stage/` with the canonical target ZIP under `/tmp/revcl-vuc-stage/archive/upstream/vuc/downloads/`. No copies or generated files were written into the other agent's active VCL worktree.

Independent strict gates on the VM snapshot:

- `match_candidates.py`: **207/207 exact functions**, **16,723/16,723 original-offset instruction bytes**; **PASS**.
- `match_partial_checkpoints.py`: **36/37 authenticated original-length partial functions**, **7,183/11,134 original-offset bytes** (within those partial functions only), **zero regression failures**; the remaining `SymbolTable::Enter` candidate is explicitly wrong-sized and **UNVERIFIED**.
- Named-function inventory: **207 EXACT, 37 EXPERIMENTAL, 54 UNRESOLVED** out of 298. Six symbols are zero-sized; the smallest-first nonexact work queue has 85 positive-size candidates.
- **Whole-ELF score: UNAVAILABLE**. Neither 16,723 exact-function bytes nor 7,183 partial bytes represent a percentage of all linked binary bytes. Do not conflate them.

VM verification commands (snapshot paths, not the unmounted clone):

```sh
limactl shell revcl-oracle -- sh /tmp/revcl-vuc-stage/vuc/scripts/restore_cached_toolchain.sh
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-rebase-exact-20261120
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_partial_checkpoints.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-rebase-partials-20261120
```

The VM's `/tmp` paths are scratch and may disappear on restart. Refresh the test snapshot from the isolated clone after changes; never take VM output as a source repository copy.

## Next source reconstruction priority

Focus **only on VUC**, and aim for another **complete original-length, relocation-verified exact function**. Read `vuc/AGENTS.md`, `vuc/README.md`, `vuc/docs/exact-match-contract.md`, and the historical negative experiment documentation before picking a new compiler hypothesis.

Strong candidates, with honest status:

1. `__uninitialized_copy_aux<normal_iterator<SymbolScope*>>` at **0x0806a840**, original **149 bytes**, currently UNRESOLVED. An authentic original-source-unit build emits **145 bytes**, with the same six external call relocations. The original has **four bytes of exception-landing alignment** not reproduced by the candidate. Many flag-only alternatives have already failed; investigate actual original C++03 template/EH ownership and predecessor relationships rather than padding. See `vuc/docs/2026-10-15-symbol-node-alias-and-real-script-diagnostic.md` and `vuc/docs/2026-10-15-symboltable-real-findcopy-source-unit.md`.
2. `Value::operator+` at **0x0806e9b0**, **203/219** original-length partial. The real vector-lane update order is **3,1,2,0**, with authentic `TypeConvert` reference; prior probes already explored broad source spelling variants. Continue only with new causal GCC 3.3.3 register/CFG evidence.
3. `Print(ASTFunCall*, ostream&)` at **0x0805b770**, **96/98** original-length partial, seven verified relocations, nine verified literal bytes. Residual ECX/EAX register bytes at +23 and +29 were repeatedly resistant to aliases. Consider genuine source-unit predecessor context, not another register alias sweep.
4. The compiler-generated Instruction arena `__tcf_1` at **0x08051980** is **45/46**. Its sole mismatch is a *dead pop-register byte* in the epilogue; dozens of experiments are exhausted. Do not repeat them without a new original-unit dependency.

Keep strict accepted C++ in `vuc/src/`, original-length partials in `vuc/probes/`, manifests in `vuc/config/` and authentic-source evidence in `vuc/docs/`. No inline ASM, binary blobs, artificial padding, fabricated addresses/relocations, or hand-tuned instruction bytes. Run both exact/partial and host tests before accepting a change; preserve 207 existing exact matches and all authenticated data/BSS evidence.

Work only inside the isolated VUC clone while another agent uses the original `main` checkout; no worktrees/subagents, no push. Commit VUC-only improvements. If the original repo's `vuc` ref must follow a new isolated-clone commit, use an explicit guarded fetch + `update-ref` while the original checkout remains on `main`.

## Follow-up compiler controls (same date)

The isolated clone now contains two reproducible source/compiler control scripts
and their results in `vuc/docs/2026-11-20-eh-alignment-and-prologue-return-controls.md`.
The actual `SymbolTable.cpp` unit's 145-byte Scope iterator copy has a
92-byte exact original-offset prefix and all six authentic relocations;
the remaining four-byte length gap is an original EH landing alignment.
Neither 26 i386 CPU-target compilations nor 16 authentic explicit-template
instantiation placement compilations nor the already investigated
alignment choices yielded the original 149-byte function. Twelve genuine
`AsmOutput_WritePrologue` return-contract/receiver controls likewise did not
improve its 22/30-byte partial. No source candidate was promoted. The
207/207 strict exact gate, 36/37 partial checkpoint gate with zero regression
failures, 244/244 initialized-data gate, 12/12 BSS declaration gate and
205/205 host tests were rerun successfully; native whole-ELF byte coverage
remains unavailable. No push was performed.

## VCL technique-transfer follow-up (same date)

Additional historically grounded VUC-only probes of original VCL
compiler-state lessons are recorded in
`vuc/docs/2026-11-20-vcl-technique-transfer-followup.md`.
Twelve campaigns produced 212 compiler builds, including true GCC3.3
source-unit/template emission controls, branch-frequency experiments
and strict sibling preservation checks. None newly matched a complete
VUC function. The strongest new mechanism is that genuine
`std::vector<Instruction*>` template placement rotates the
`Instruction::__tcf_1` dead cleanup pop from EBX to EAX without
affecting its other 45 matching bytes. The real exact **265/265**
Instruction vector helper and **45/46** arena callback also coexist
in one compiled source-unit candidate; the original ECX pop is still
missing. Moving the genuine `Instruction::instrData` first strong
definition recovers the original startup symbol name but only **29/31**
startup bytes. Do not mistake an existing exact sibling for a newly accepted
function. The original ZIP and canonical VCL checkout remain untouched;
no push was made.

# Resume VUC — historical GCC 3.3.3 byte-exact reconstruction

Continue the VUC 2004.1 ELF32-i386 decompilation/reconstruction in the REVCL repository, **not VCL**. Another agent is independently working on VCL in the canonical checkout, so **do not switch or modify its active `main` checkout, clean its untracked files, create Git worktrees, or use subagents**.

**Workspace:** `/local-home/Projects/.revcl-vuc-rebase-20261120` (separate full clone on branch `vuc`, with `origin` pointing to `git@github.com:fjtrujy/revcl.git`). The canonical repository is `/path/to/revcl` and its local `vuc` reference has already been rebased to `origin/main` from 2026-11-20. The isolated clone is the safe place to continue now. Do not push.

## Previous work already complete

- Rebased all 191 VUC commits on `origin/main` at `47e0c6fb`, with no conflicts; the `vuc/` tree remained byte-identical. The original VUC branch HEAD was `2109c25c`. The first rebased HEAD was `c8f585a4` (see `git log -1` for the latest checkpoint commit).
- Original immutable VUC target ZIP `archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip` contains a **314,338-byte ELF32 i386** original, 298 named text-function symbols. Use only original canonical bytes and verified relocations.
- Verified **207/207 exact candidate functions, 16,723/16,723 original instruction bytes** using historical GCC 3.3.3 20040125 prerelease and GNU as 2.15. Host tests **205/205 passed**.
- Verified separate partial manifest **36/37 authenticated original-length experiments, 7,183/11,134 original-offset bytes, zero regression failures**; `SymbolTable::Enter` is wrong-sized/unverified. Current ledger: **207 EXACT / 37 EXPERIMENTAL / 54 UNRESOLVED**. Full linked-ELF score remains **UNAVAILABLE**; never claim it is 99% complete.
- Lima VM `revcl-oracle` boots. Cached GCC compiler lives under `~/.cache/vuc-toolchain`; `/tmp` aliases were restored. The isolated clone isn't mounted into this VM; the current VUC-only test snapshot is `/tmp/revcl-vuc-stage/`. If it vanishes, copy fresh `vuc/` and the original ZIP from the isolated clone into guest scratch, then run `vuc/scripts/restore_cached_toolchain.sh` there.

## Objective and constraints

Recover more complete, byte-for-byte exact VUC functions from **real, period-plausible C++03** using the original code/relocations, GCC 3.3.3 optimizer/RTL, authentic source-unit and template instantiation context. Do not fake a match with ASM, NOPs, copied machine code, fabricated symbol addresses, manually inserted padding, or patched scoring. For an accepted source, the compiled function must have the **exact original length and all bytes and supported relocations correct**. Keep **partial** source in `vuc/probes/`, accepted source only in `vuc/src/`, test manifests in `vuc/config/`, and negative/provenance notes in `vuc/docs/`.

Review `vuc/AGENTS.md`, `vuc/README.md`, `vuc/docs/exact-match-contract.md`, the checkpoint report `vuc/docs/2026-11-20-vuc-rebased-checkpoint.md` and the recent Oct 23 VUC research notes. Avoid repeating exhausted variations: the Instruction cleanup `__tcf_1` is 45/46 (dead pop register) and ASTFunCall printer is 96/98 (ECX/EAX), already deeply investigated with no exact match.

**Suggested next target:** `std::__uninitialized_copy_aux<normal_iterator<SymbolScope*>>` at `0x0806a840` (149-byte original; real-unit candidate 145 bytes, missing four genuine EH landing alignment bytes). It has authentic six call relocations and neighboring exact `SymbolTable` functions. First examine existing negative controls in `vuc/docs/2026-10-15-symbol-node-alias-and-real-script-diagnostic.md` and `vuc/docs/2026-10-15-symboltable-real-findcopy-source-unit.md`; find a **new causal source-unit/template/EH ownership hypothesis**, not another alignment flags sweep. If exhausted, pivot to an untried function or `Value::operator+` (203/219 authentic partial), preserving strict gates.

## Run and verify

From the isolated clone, run host tests:

```sh
/path/to/revcl/.venv/bin/python -m unittest discover -s vuc/tests -q
/path/to/revcl/.venv/bin/python vuc/scripts/smallest_first.py --top 30
```

For original GCC verification, after syncing changed VUC files to the **VM scratch snapshot**, use:

```sh
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_candidates.py --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-next-exact
limactl shell revcl-oracle -- python3 /tmp/revcl-vuc-stage/vuc/scripts/match_partial_checkpoints.py --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-next-partials
```

Log every experiment, positive or negative, with compiler flags, original length, matched original-position bytes, relocation/literal authenticity and file/unit impact. Commit VUC-only progress **locally**; do not push or switch the other agent's `main` checkout. Provide updates after each meaningful attempt and report exact/partial/whole-binary metrics separately. Continue autonomously without asking approval for each trial.

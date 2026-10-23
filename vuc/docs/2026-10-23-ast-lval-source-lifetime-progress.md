# VUC 2026-10-23 — AST L-value printer stack-lifetime and cleanup campaign

Branch: `vuc`; starting HEAD `201851f9`. Target is the unchanged VUC
2004.1 Linux x86 ELF from the canonical upstream ZIP (SHA-256
`14c77997057f279ab25a3591c6a379d6e4744f2b1551271d43602948c037d8cf`).
Use the recovered GCC 3.3.3 **2004-01-25** and **2003-12-06**
prerelease snapshots plus GNU as 2.15. Neither is authenticated as the
unrecovered Debian `0pre3` toolchain. All emitted candidate sources and
ELF objects are under `/tmp` in the existing `revcl-oracle` guest.
There is no new worktree, assembly patch, fake register directive,
source-padding trick, original ELF alteration, or production header change.

## Verified original structure

Original function:
`Print(ASTLValExpr*,std::ostream&)` / `_Z5PrintP11ASTLValExprRSo`
at **0x0805c4f0**, **765 bytes**.

Prior progress established three recursive cases with l-value tag at
`ASTLValExpr+0x1c`, a common
`matchStack` at **0x0808abe4** (12-byte `.bss`),
a genuine inline vector-style growth of 4-byte node pointers
(initial capacity **16**, subsequent capacity **2×**),
and an exception-safe depth decrement. The first source-directed
candidate at `vuc/probes/PrintASTLValRecursionOwnerOct23.cpp`
compiled to **722 bytes**, stack frame **0x48**, under both original-era
compiler snapshots.

The original instruction sequence uses:
- **0x68** byte stack allocation;
- signed count at `matchStack+0`;
- signed capacity at `matchStack+4`;
- **ASTNode\*\*** pointer-array storage at `matchStack+8`;
- `new ASTNode*[capacity]`, inline GCC memcpy/rep-movsl
  and exception-safe cleanup.
- The originally observed 3-branch call sequence uses the identifier
  `MLGameString` at `+0x24`, member and index fields at
  `+0x24/+0x28`. The exact original class source is not yet known.

The AST pointers are shared by all four original printers, so
`matchStack` cannot legitimately be represented as a type-specific
`ASTLValExpr**`. The probe now uses `ASTNode**` and takes
`ASTNode*` in its insertion method, consistent with the original
RTTI inheritance chain.

## Concrete source-fidelity gain in the already exact AST.cpp

The accepted `vuc/src/AST.cpp` had represented the three fields
as three unrelated `void*` pointers. Those happened to compile to
the exact original zeroing stores but did **not** describe how
the object is used at runtime by any of the four printers.

Changed the source declaration to:

```cpp
struct ASTNode;
struct MatchStack {
    int length;
    int capacity;
    ASTNode **entries;
    MatchStack() : length(0), capacity(0), entries(0) {}
};
```

This is not inferred merely from 12 zero bytes: it follows from the
actual original ELF's signed length/capacity comparisons, 16-element
pointer allocation, pointer stores, and indexed 4-byte copy operations.
Source ABI/layout is still 12 bytes.

First reconstructed this change in a **scratch** original translation
unit and compiled with
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`.
The old exact sibling functions remained completely exact:
- `__static_initialization_and_destruction_0`: **145/145**,
  11/11 authentic relocations.
- `_GLOBAL__I_matchStack`: **31/31**, 1/1 authentic relocation.

Only after that check was the verified type/name correction applied
to `vuc/src/AST.cpp`. The remaining source in `vuc/src`
and all exact-candidate manifests were unchanged.

## 554 genuine historical GCC compilations

The source-reproduction family includes **354 candidate source shapes**;
Jan25 compiled all 354 and Dec06 compiled 200 high-value/protected
variants (plus baseline checks). This is **554 successful compilations**.
The Jan25 and Dec06 results agree wherever both were run.

| Reproducer | Source cases | Valid builds | Size observations |
| --- | ---: | ---: | --- |
| `evaluate_lval_real_lifetimes_oct23.py` | 90 | 180 | Original 722, live child-pointer crossings to **755** |
| `evaluate_lval_cleanup_order_oct23.py` | 126 | 176 | Natural cleanup checks/branch orders, **747–786** |
| `evaluate_lval_original_selector_cfg_oct23.py` | 48 | 56 | Nested original-like two-stage guards, **731–770** |
| `evaluate_lval_real_guards_oct23.py` | 18 | 30 | Genuine identifier, stream and punctuation value lifetimes, **738–755** |
| `evaluate_lval_guard_owner_oct23.py` | 40 | 72 | Scope guard owning `MatchStack&`, pointer or node; **738–862** |
| `evaluate_lval_default_arms_oct23.py` | 32 | 40 | Explicit semantic no-op/default switch arms, **747–779** |

All six scripts and the original, non-accepted probe are retained
in `vuc/probes`. Per-case complete source, objects, and machine-size
JSON ledgers are persisted under corresponding
`/tmp/vuc-ast-lval-*-20261023` or
`/tmp/vuc-lval-*-20261023` directories in the guest.
The scripts compare original lengths using symbol sizes emitted by the
actual GCC snapshots (not assemblers with forced bytes or a modern
compiler). The snapshots were first tested on actual original code
relocations, and the existing accepted Jan25 compiler was left untouched.

### Meaningful compiler observations

1. **Liveness across the real allocation:** preloading both genuine
   child pointers of the member/array case **before**
   `matchStack.Add(node)`, and consuming those saved values
   **after** its potentially throwing `new[]`, makes the compiler
   preserve more live values. The natural `entries[length++] = node`
   store changes the register web further. The resulting source has
   the original **0x68 stack frame** and **755 bytes of code**,
   compared with **0x48/722** from the old probe. That closes
   **33 of the 43 missing code-size bytes** without artificial code.
   It does **not** establish equal instruction bytes.
2. **Dispatch shape:** a nested `if(kind != 0)` /
   `if(kind > 0)` body gives GCC the *same kind of two-test
   early branch structure* visible in the original; a conditional
   `length==capacity` growth shape then emits **763 bytes**.
   This equality guard has narrower semantics than the original's
   signed less-than test unless the ordinary capacity invariant
   holds; do **not** promote it as authenticated source.
3. **Cleanup behavior:** natural alternatives
   `if(length > 0)`, `if(length >= 1)`, a saved counter, and
   `if(length != 0)` change body size in combination with
   different case orders. Some variants emit **766** bytes, but
   `length != 0` is not equivalent to the original signed
   positive-depth check for a negative/corrupted count, so the
   shorter distance to 765 must not be confused with a better
   reconstruction.
4. **Owning guard class:** changing the RAII guard to store
   `MatchStack&`, `MatchStack*`, an actual entered
   `ASTLValExpr*`, or combinations is real C++03 source, but
   grows/recolors the EH path significantly (some emit 789+ bytes).
   The original guard representation remains unproven.
5. **Shared node type:** replacing a specific L-value pointer
   array with the correct `ASTNode**` preserves the code sizes
   in the paired controls. This is a semantic correction, not a
   new exact-byte win.

**No variant emits the original 765-byte length.** The closest
generated lengths include **763**, **766**, and **767**, but those
are *size-only diagnostic measurements*. No original-length
candidate was authenticated for relocation-resolved byte equality;
the selected `vuc/src` function count must not change.

## Strict acceptance readiness

Created
`vuc/config/print_ast_lval_recursion_oct23_probe.json`,
anchored to the authenticated original function symbol, original VA
and length. Relocation names map to actual original direct call
targets: `operator new[]`, the two recursive printer overloads,
`MLGameString::c_str`, ostream insertion, `_Unwind_Resume`,
and the original `matchStack` object. Its 6-byte literal
map uses **literal bytes verified against the original ELF**:
- `".\\0"` at 0x080766fb;
- `"[\\0"` at 0x08072712;
- `"]\\0"` at 0x080726c9.

`match_probe.verify_literal_ranges` independently verified
**all 6/6 bytes** for the preserved C++03 object. The source
symbol `matchStack` was separately authenticated at
**0x0808abe4**, 12 bytes, `.bss`; the LVal and Expr functions
at **0x0805c4f0/765** and **0x0805c0a0/1089** were also
authenticated against the original `.symtab`.
The strict comparator will additionally validate every
relocation and require **exact original length** before it
returns an accepted original-offset match.

No candidate has reached that threshold in this batch; an
overlapping-prefix score of a wrong-sized body would be misleading
and has deliberately not been promoted.

## Validation and remaining work

The **full 205-function recovered-GCC exact acceptance** and
the **199-host-test suite** were rerun after the semantic
`AST.cpp` source change. The separate full original-BSS gate
also confirms `matchStack` retains the correct declaration,
section and 12-byte size. The whole original ELF cannot yet
be linked and matched byte-exact; exact functions, accepted data,
and BSS declarations are separate coverage metrics.

Final independent acceptance results after the source change:

| Verification | Result |
| --- | --- |
| Original-length byte-exact functions | **205/205**, 16,093/16,093 instruction bytes |
| Named initialized data objects | **244/244**, 71,536/71,536 bytes |
| Original BSS declarations | **12/12**, 18,437 source allocation bytes |
| Python host suite | **199 passing tests** |

The host suite's separate deliberate negative exact-match fixture
correctly remains rejected; that expected fixture result does not
represent a failed main regression. The whole-ELF score remains
unavailable until a byte-exact link is reproduced.

The next focused investigation should compare the **actual
.21/.24 RTL register def-use chains** of the 755-byte,
0x68-frame candidate and original for the first
`matchStack.Add` allocation; recover source that retains
the original old-count spill without incorrectly preloading
the child fields. Also compare the original, naturally
ordered parent-case guard and positive-count cleanup
to reduce the 10 remaining bytes in a semantics-preserving
way. Only then run strict symbol/relocation/literal/byte
acceptance and consider adding the function to `vuc/src`.

Reproduce on the existing VM (these scripts regenerate scratch;
do not treat the generated size as a byte match):

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_lval_real_lifetimes_oct23.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_lval_cleanup_order_oct23.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_lval_original_selector_cfg_oct23.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_lval_real_guards_oct23.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_lval_guard_owner_oct23.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_lval_default_arms_oct23.py"
limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-lval-exact-gate
python3 -m unittest discover -s vuc/tests -q
```

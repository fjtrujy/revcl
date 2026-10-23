# VUC: preserve authenticated partial byte matches between iterations

**2026-10-23** · branch `vuc` · starting commit `7ce39aaa`.

The byte-exact goal is unchanged. The historical VUC executable is immutable;
the recovered January 25, 2004 GCC 3.3.3 prerelease and GNU as 2.15 are used
for comparisons. The recovered compiler remains a historically compatible
approximation rather than an authenticated Debian `0pre3` binary.

## Two independent kinds of progress

1. **Fully exact, accepted C++03 source:** `vuc/src/`, listed in
   `vuc/config/exact_candidates.json`, subject to the existing mandatory
   `match_candidates.py` acceptance suite. No merely partial function is
   allowed in this list.
2. **Original-offset partial-byte working checkpoints:** `vuc/probes/`,
   listed in `vuc/config/selected_candidates.json` but **not** in the
   exact manifest. A function gets a partial byte score only if the historical
   compiler emits **exactly the original function length**, the original
   function identity and address are authenticated, all emitted relocation
   sites are resolved against original symbol addresses, and all emitted
   literal bytes map to authenticated original bytes. A wrong-length
   function, including a close 755-byte body versus 765 original, has
   **no admissible partial byte score**. It can still be saved as a useful
   *structural* experiment.

New command `vuc/scripts/match_partial_checkpoints.py` compiles every
existing **selected minus exact** candidate with its own recorded original
compiler flags, using `vuc/scripts/match_probe.py` for strict original-ELF
comparison. It writes a scratch report, including original-offset differing
indices, compiler flags, source SHA-256, symbol identity, relocation count,
literal verification count, and failures. Identical source/flag compilation
recipes are shared. It never alters `vuc/src` or exact acceptance.

`vuc/config/partial_byte_checkpoints.json` preserves the measured lower
bounds per function. Unless explicitly refreshed, a future run **fails**
if any previously verified partial loses byte matches, loses its original
length, becomes unrelocatable, changes the original symbol identity, or loses
authenticated literal coverage. Genuine improvements are reported separately;
raise the pinned floors with `--refresh-baseline` only after review.
Even an explicit refresh refuses an accidental downgrade/disappearance.

## First real VUC checkpoint ledger

Validated with the recovered January GCC 3.3.3 snapshot:

- **38/39** selected experimental functions have authentic original-length
  and original-offset comparisons.
- **7,783/11,764** verified bytes *inside those 38 selected partial functions*,
  not a whole-binary score and not bytes gained since the previous commit.
- The 39th, `SymbolTable::Enter` at `0x0806a330`, is **UNVERIFIED**:
  its present C++ source compiles to **194 bytes**, versus **185** original.
  Its partial byte count is not included in either total.
- Sample protected checkpoints include
  `std::vector<SymbolScope>::_M_insert_aux` **733/746**,
  `Print(ASTFunCall*, std::ostream&)` **96/98**,
  the 46-byte Instruction cleanup function **45/46**,
  and `MLTempString::Set` **111/117**.
- **No** experimental checkpoint may be promoted to accepted exact
  source without achieving full original-length and full byte equality.

This is an additional diagnostics inventory; the existing baseline remains
**205/205 exact functions** and **16,093/16,093** accepted original instruction
bytes. The separate initialized data, BSS declaration and whole-linked-ELF
gates are not conflated with these partial byte counts.

## Preserved LValue working source

`vuc/probes/PrintASTLValLiveChildrenOct23.cpp` stores the best
**semantically motivated source-level** direction from the prior 554-build
campaign in a single, permanent C++03 file. It evaluates both real array
and member child pointers *before* the potentially throwing insertion into
the global `matchStack`, then consumes them *after* that insertion.
The insertion stores via `entries[length++] = node` and its shared
storage uses `ASTNode**` rather than an incorrect LValue-only array.

Both historical compiler snapshots generate **755 bytes** for this
function, versus original **765**, and the closer candidate uses
the original **0x68** stack frame. This source is a **structural,
wrong-length checkpoint**, not one of the 38 authenticated byte
matches and not an exact reconstruction. It is deliberately excluded
from `selected_candidates.json` until it can pass strict
length/relocation/literal verification. The ELF-anchored
`vuc/config/print_ast_lval_recursion_oct23_probe.json` is ready for
that moment.

## Reproduction and next actions

From the existing project checkout:

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- python3 "$R/scripts/match_partial_checkpoints.py" \
  --compiler "$C" --out /tmp/vuc-partial-checkpoints-oct23
# Only after reviewing verified gains:
limactl shell revcl-oracle -- python3 "$R/scripts/match_partial_checkpoints.py" \
  --compiler "$C" --out /tmp/vuc-partial-checkpoints-oct23 \
  --refresh-baseline
limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" \
  --compiler "$C" --out /tmp/vuc-exact-gate-with-partials
python3 -m unittest discover -s vuc/tests -q
```

On the next `ASTLValExpr*` iteration, edit the persisted 755-byte
source, not only an ephemeral `/tmp` variant. Recompute compiler
function length and test the strict original ELF relocation/literal
manifest as soon as a 765-byte variant emerges. Preserve source files
for **verified increases in original-offset matching bytes**, not
only perfect completions. Never advance the exact-function manifest
based on an improved partial or an unaligned byte overlap.

# VUC next batch: IR constructor negatives and BroadcastMove 191/416

Date: 2026-10-15, branch `vuc`, original VUC 2004.1 Linux i386.
Only original ELF function records and historical GCC 3.3.3 snapshots
are accepted as byte-matching evidence. There are 155 unrelated
pre-existing untracked paths; none were staged or modified.

## Canonize_FoldExpr: test real C++03 constructor ownership

The prior first-source `vuc/probes/CanonizeFoldExprNewOct15p.cpp`
recovered the original polymorphic 12-byte `ConstLit_Data` and
16-byte `ConstExpr_Data` allocation behavior at `0x080502c0`,
but its output was 381 bytes vs. the original 410.

New repeatable `vuc/probes/evaluate_canonize_fold_ctor_layout_oct15q.py`
tests the exact constructor ordering hypothesis suggested by original
allocation disassembly: the original writes kind at object +4 before
writing the derived virtual table at +0, while the partial constructor
initializes these in a different order. The five distinct C++03
base/derived source lifetime forms include:

- `Expr_Data(unsigned kind):kind(kind)`, with derived member-init
  lists `Expr_Data(7)` or `Expr_Data(6)`;
- base constructor assignment `kind=kindValue;`;
- derived body assignment in lieu of initializer lists;
- mixed base-kind initialization and derived body member writes.

All source forms compile with both Jan25 and Dec06 GCC 3.3.3 and GNU
as 2.15. **Every version remains 381 bytes at -O2
-frename-registers**; none is the original 410. Thus ordinary
base-versus-derived initialization spelling alone cannot explain
the missing original source instructions. There is **no original-length
candidate** and `Canonize_FoldExpr` remains unresolved, not promoted
to the exact or experimental manifests. Sources/ELF32 objects and
negative results are retained under
`/tmp/vuc-binutils215/canonize-fold-class-constructors-oct15q/`
in the historical Linux guest.

Future improvement requires authentic adjacent `IR` object
constructors, expression ownership, or prior TU functions rather than
more forms of the same optimizer-equivalent initializer.

## BroadcastMove: full original channel source and original-length partial

Original VUC function:
`BroadcastMove(IR::Register const&, IR::Register const&,
InstructionList&)` at **0x08051b30**, **416 bytes**.

The accepted original source-unit context
`vuc/src/IR2AsmBroadcastUnit.cpp` already owns a real exact startup
wrapper, but its application function body was incomplete. This batch
does not alter that accepted unit. A new experimental
`vuc/probes/BroadcastMoveFullReconstructionOct15q.cpp`
first recovers the missing z-channel and fourth empty temporary
`MLTempString` concatenation; this restores the source-defined
five-temporary channel construction. Its original-size candidates
are not yet found (at default -O2, 403 bytes).

The original ELF reveals an unusual, apparently redundant
control-flow branch:

- Before the `mul.w` case it tests `register.channels & 7`,
  then `register.channels & 8`.
- In the channel-composition branch, it computes
  `mask = register.channels & 7`, compares **mask == 15**
  (impossible after `& 7`), then tests `mask == 0`.
- The fallback and composed-channel path join at a **single**
  `InstructionList::Add("add%s %s, vf00, %s",...)` call.
- It calls `input.name.c_str()` and `result.name.c_str()`
  *before* allocating the suffix temporaries and copies the
  `IR::Register` return object only on the common exit path.

This matches the period's possibly imperfect human C++03 coding
style; the absurd comparison is preserved instead of optimized away
by hand. The two source-control programs
`evaluate_broadcast_channel_branch_oct15q.py` and
`evaluate_broadcast_authored_mask_branch_oct15q.py` demonstrate
that simply rephrasing the Boolean branch leaves GCC's original
304..403-byte form unchanged, while adding two independent
`InstructionList::Add` paths inflates the output to 448..478
bytes. Neither is the original.

A final genuine-source variant,
`vuc/probes/BroadcastMoveCommonAddOct15q.cpp`, preserves
the original idiom and shares one `Add` call, returning the result
by value. GCC 3.3.3 now emits the **original 416-byte length
and the original 0x48-byte stack frame**, in both Jan25 and Dec06
snapshots using
`-O2 -frename-registers -falign-jumps=4`.

### Strict byte, literal and relocation gate

The new selected **experimental-only** config is
`vuc/config/broadcast_move_partial_oct15q.json`.
Historical GCC original-offset comparison reports **191/416
instruction bytes**, **all 27 relocation sites**, and
**all 47 generated literal bytes** authenticated against the linked
original ELF. This includes the literal bytes and
sub-string sharing of:

| Source literal | Original ELF address |
| --- | --- |
| `mul.w %s, vf00, %s` | `0x0807267f` |
| `add%s %s, vf00, %s` | `0x08072692` |
| empty string | `0x08072286` |
| `.` | `0x080766fb` |
| `x` | `0x080721d8` |
| `y` | `0x080722e8` |
| `z` | `0x08072c3a` |

Named call relocations include
`MLGameString::MLGameString(const MLGameString&)`,
`MLGameString::c_str()`,
`InstructionList::Add(char const*, ...)`,
`MLTempString::MLTempString(char const*, ...)`,
and `MLTempString::operator+=(const MLTempString&)`.
The authentic original address for every such named call is verified
in the config, and the original `.rodata` spans are checked byte
by byte. The register-allocation prologue and some branch displacement
bytes still differ, so **191/416 is not an exact function**.

The previous count was **204 EXACT / 32 EXPERIMENTAL /
62 UNRESOLVED**; this original-length, full-relocation partial
brings it to **204 EXACT / 33 EXPERIMENTAL / 61 UNRESOLVED**
across 298 original function records. There is no whole linked ELF
match.

### Reproduce in the existing guest

```sh
R=/path/to/revcl/vuc
/tmp/vuc-binutils215/g++-jan25-as215 \
  -O2 -frename-registers -falign-jumps=4 \
  -c "$R/probes/BroadcastMoveCommonAddOct15q.cpp" \
  -o /tmp/vuc-binutils215/broadcast-common-selected.o
python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/broadcast-common-selected.o \
  --config "$R/config/broadcast_move_partial_oct15q.json"
```

The full unselected source-control experiments and generated ELF32
objects are stored outside Git under
`/tmp/vuc-binutils215/broadcast-channel-flow-oct15q/` and
`/tmp/vuc-binutils215/broadcast-authored-mask-oct15q/`.
Do not claim that an original-length candidate is accepted until
**all 416 instruction bytes** reproduce.

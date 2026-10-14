# VUC: Recover byte-exact glibc init startup alongside fini

Date: 2026-10-14. Branch: `vuc`. Baseline `d1149a2`.
Original: unmodified upstream VUC 2004.1 Linux ELF32 i386, verified
against the historical VUC archive. Compiler evidence uses the recovered
2003-12-06 and 2004-01-25 GCC 3.3.3 prerelease C frontends, GNU as 2.15.

## New exact function

**`__libc_csu_init` at `0x08071f70`: 84/84 exact instruction bytes, 5/5
original PIC relocation sites**, promoted into `vuc/src/LibcCsu.c`.

The original `__libc_csu_fini` at `0x08071fd0` remains
**80/80 bytes and 5/5 PIC relocations exact**. Critically, these
functions were verified *together from a single C translation unit*,
not by linking independent copies or faking symbol locations.

The restored ordinary C source for init is:

```c
unsigned int i = 0;
_init();
const unsigned int count = __init_array_end - __init_array_start;
while (i < count) {
    (*__init_array_start[i])();
    ++i;
}
```

The original function initializes its loop index before `_init`,
reads the difference of the glibc init-array endpoints and iterates
upwards, invoking each hook. This straightforward lifetime and C
control-flow choice recovers the originally missing register-save
pattern, compare/loop branches and function return. No inline
assembly, dummy code, zero filling, instruction patching or invented
addresses were introduced.

Both original-era compiler snapshots produced **both exact functions**
when compiling the same recovered file under **`-xc -O2 -fPIC`**,
*without a per-unit alignment exception or register-renaming override*.
Adding `-falign-jumps=1` does not alter either exact body, but is
not needed and was removed from the accepted init manifest.

Relocation addresses checked against the unchanged canonical binary:

| Relocation | Authenticated original address |
| --- | --- |
| `_GLOBAL_OFFSET_TABLE_` | `0x0808a668` |
| `__init_array_start` | `0x08077000` |
| `__init_array_end` | `0x08077000` |
| `_init` | `0x080495c4` |
| PC-thunk/GOT setup | Calculated from original ELF / i386 PIC contract |

The `R_386_GOTPC`, `R_386_GOTOFF` and `R_386_PLT32`
relocations are resolved by the existing strict VUC ELF matcher.
`__libc_csu_fini` retains its verified GOT, fini-array and
`_fini` relocations. A pair of 84/84 or 80/80 machine-code
counts without those relocations is insufficient and was not accepted.

## How it was recovered

Driver: `vuc/probes/evaluate_csu_init_source_ownership_oct14.py`.
Started with the existing original C startup reconstruction, recovered
from the historical array lifetime and glibc startup idioms. It
compares candidate ordinary C bodies including index-before-init,
postincrement loops, initial guard, downward loops, base-pointer
lifetimes, and const count after `_init`. Compiler matrix uses 12
historic flag configurations and both GCC 3.3.3 snapshots; it stops
on authenticated full equality. The January compiler first found
the exact result in the **const-count, while-loop** candidate
(`cache-count`) with `-falign-jumps=1`. Subsequent independent
builds establish **`-xc -O2 -fPIC`** suffices and demonstrate that
the exact result does not depend on the sample's formatting or
separate translation-unit placement.

The accepted config `vuc/config/libc_csu_init_pic_probe.json`
now points to `vuc/src/LibcCsu.c` and is in both
`vuc/config/exact_candidates.json` and
`vuc/config/selected_candidates.json`.

## AST expression-list negative experiment

The earlier experimental 112/256 `Translate(ASTNodeList<ASTExpr*>&)`
candidate remains unchanged. A separate bounded source-level
investigation,
`vuc/probes/evaluate_ast_expr_rep_movsl_oct14.py`, tested
**540 compiler recipes** of `__builtin_memcpy`, `memmove`,
`std::copy`, owned old pointer/size and new-capacity update order.

GCC 3.3.3 indeed emits the original family of **`cld; rep movsl`**
instructions from ordinary `__builtin_memcpy` on the pointer-array
elements; this is stronger structural evidence than a scalar-copy
approximation. However, the best semantic source forms emitted
**249-byte** code (seven bytes shorter than the 256-byte original)
under the tested options. None of these 540 candidates had the
original length **and** authenticated relocations; no byte-exact
claim or manifest change was made. The rejected copy alternatives
and their original-era objects remain in the VM scratch output.
Future work must recover genuine index/copy allocation lifetimes
rather than add artificial executable padding.

## Regression and bounds

The 298 named original VUC functions are now **198 EXACT /
19 EXPERIMENTAL / 81 UNRESOLVED**. Function-level exact instruction
coverage increases from **14,417** to **14,501** bytes, with no
previously exact function displaced. The previously verified
244 initialized-data objects and 12 BSS symbol declarations must
remain exact in the full gates; user files and untracked work stay
preserved. This new result does **not** establish whole-file ELF
byte-for-byte identity or exact original source ownership beyond the
reconstructed, jointly compiled startup function pair.

## Reproduction

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215

limactl shell revcl-oracle -- "$C" -xc -O2 -fPIC \
  -c "$R/src/LibcCsu.c" -o /tmp/vuc-binutils215/csu-init-verified.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/csu-init-verified.o \
  --config "$R/config/libc_csu_init_pic_probe.json"
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/csu-init-verified.o \
  --config "$R/config/libc_csu_fini_pic_probe.json"
```

The same invocation with `g++-dec06-as215` instead of the January
compiler also verifies exactness of both functions. No existing
worktree, VCL project source or original archive was modified.

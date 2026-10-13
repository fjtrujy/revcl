# VUC rebase onto VCL main a0b6acf and Flex buffer partial

Date: 2026-10-13. Branch: `vuc`. Historical target: the original
VUC 2004.1 Linux x86 ELF inside the pinned upstream ZIP, untouched.

## Rebase and updated cross-project compiler evidence

The checkout began at `3ccb4c4` with no tracked changes and **146**
pre-existing untracked paths. `git fetch origin main` found three new
VCL commits after `bbe5056`:

- `794c9eb`: bounded failed-round observations and compiler-pass notes.
- `ab46619`: byte-exact `checkInputLock.cpp` in VCL.
- `a0b6acf`: recovered `dynCalcStats.cpp` source context,
  including a reproducible measurement matrix.

Local `main` was updated to `a0b6acf`. The **101 VUC commits**
rebased cleanly onto it, producing starting VUC HEAD `d5507f6`;
no conflicts or untracked-path collisions occurred.

The new `config/matching-evidence/dyncalcstats-split.md` from main
explains why authentic predecessor context and *indexed spelling*
inside a loop can change postreload/peephole2 and register assignment.
An exact VCL control matched after a **user-authorized but not
historically proven compilation-unit split**. This is a valuable
methodological lead, not evidence that VUC shared the VCL original
source layout. **VCL uses GCC 3.2.2 while VUC uses historical GCC
3.3.3 prereleases**, so optimizer effects must be checked independently.

VUC source, original binaries, exact code, data and BSS manifests were
preserved through the rebase; root VCL sources were not edited.

## Genuine yyFlexLexer buffer reconstruction: 430/550 partial

Original function:
`yyFlexLexer::yy_get_next_buffer()`,
address **`0x080611c0`**, **550 original instruction bytes**.

Experimental C++03 source:
`vuc/probes/yy_get_next_buffer_partial_oct13.cpp`.

Experimental manifest:
`vuc/config/yy_get_next_buffer_oct13_probe.json`.

Recovered January 25, 2004 GCC 3.3.3 and GNU as 2.15 command:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  /tmp/vuc-binutils215/g++-jan25-as215 \
    -O2 -frename-registers -falign-jumps=1 \
    -c "$R/probes/yy_get_next_buffer_partial_oct13.cpp" \
    -o /tmp/vuc-binutils215/yy-next-partial-final.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/yy-next-partial-final.o \
    --config "$R/config/yy_get_next_buffer_oct13_probe.json"
'
```

Result: **430/550 matching original instruction bytes**, exactly
550-byte original function length, **four validated machine-code
relocations** (three original string pointers and one authentic
`realloc@plt` at `0x0804988c`). The loop/EOF handling, virtual
`LexerInput`, `LexerError` and `yyrestart` calls use the verified
scanner vtable slots and original buffer-field offsets.

Three original error strings are independently authenticated directly
from the upstream ZIP-backed ELF:

| Original literal | Length incl. NUL | Original address |
| --- | ---: | --- |
| `input in flex scanner failed` | 29 | `0x08075ba6` |
| `fatal error - scanner input buffer overflow` | 44 | `0x08074d40` |
| `fatal flex scanner internal error--end of buffer missed` | 56 | `0x08074d80` |

**Literal-provenance limitation:** the standalone GNU C++ object emits
**152** `.rodata` bytes: the 129 authenticated string bytes plus **23
padding zeros**. The experimental relocation-literal manifest verifies
those zeros against confirmed original zero padding at `0x08074d6c`,
so the checker reports **152/152 bytes verified**. Reusing that
original zero interval for candidate alignment bytes is *not* a claim
that the actual original compilation unit placed those padding
fragments there, or that the original layout is reconstructed.
This candidate **must not enter the exact manifest** without a
historically grounded literal-layout proof and 550/550 code match.

### Bounded negative controls

- `vuc/probes/yy_get_next_buffer_oct13.cpp`: initial natural generated
  Flex buffer copy, EOF handling, virtual input and ownership-aware
  `realloc` source, originally compiled to 580 bytes under
  `-O2 -frename-registers`. With `-falign-jumps=1`,
  the function emits exactly 550 bytes and **425/550** match.
- `vuc/probes/evaluate_yy_buffer_growth_oct13.py`: nine
  real C++ buffer-size growth and old-pointer lifetime forms
  under four historical optimizations. The *saved old buffer pointer
  plus compound signed-size update* is both original-length and
  the **best 430/550**. A local-allocation-result form retains
  425/550; removing `-frename-registers` changes the structure.
- `vuc/probes/evaluate_yy_get_next_buffer_alignment_oct13.py`:
  bounded jump/label/loop alignment variants for the 430-byte partial.
  None improved it. Some 550-byte settings produced only 123
  matching bytes, because their loop/branch placement changed; many
  other settings emitted non-original-size methods.

The original and reconstructed opening, copy and EOF paths agree
closely; most mismatches involve the growth/ownership path, register
liveness across `realloc`, and jump targets/padding. This is a
**meaningful new partial**, not byte-exact. The original 550-byte
method is substantially larger than its four now exact Flex
neighbors (161, 205, 170 and 167 bytes), which provide independent
grounding for buffer structure and virtual interface.

## Regression and scope

The VUC strict baseline remains **194 exactly accepted functions**,
**13,847/13,847 accepted instruction bytes**, **244/244 named
initialized-data objects with 71,536/71,536 bytes**, and **12/12 BSS
declarations**. The new named-function ledger is **194 EXACT,
16 EXPERIMENTAL, 88 UNRESOLVED**, out of 298 original named functions.
No full linked ELF byte identity is established.

The negative controls do not edit original bytes, invent external
function addresses, emit inline assembly, manually pad C++ with NOPs,
or modify root VCL code. The rebase preserves all prior exact
functions independently; this partial remains in `vuc/probes/`,
not `vuc/src/`.

For the next attempt, vary the actual C++ growth/ownership
predecessors and compiler unit context, *not* fabricated scratch
register instructions. The original's first relocated and x87-free
regions are largely accounted for; the remaining branch/control
differences should be traced from the original buffer-growth CFG.

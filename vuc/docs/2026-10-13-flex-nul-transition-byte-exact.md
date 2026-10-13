# VUC Flex NUL transition: 161/161-byte exact reconstruction

Date: 2026-10-13. Branch `vuc`. Baseline `d4ed221`.
Original target: `yyFlexLexer::yy_try_NUL_trans(int)`, address
`0x080614c0`, original function length **161** bytes.

## Byte-exact result

Source: `vuc/src/YyTryNulTrans.cpp`.
Accepted config: `vuc/config/yy_try_nul_trans_oct13_probe.json`.

The recovered January 25, 2004 GCC 3.3.3 and GNU as 2.15 compile
the source under **`-O2 -frename-registers`** to **161/161
identical original instruction bytes**, with all **eight**
relocations resolving to original ELF addresses. The independent
December 6, 2003 GCC 3.3.3 snapshot also generates 161/161
under the same options. This byte identity does not identify which
compiler snapshot produced the original scanner source.

All six referenced original scanner tables are external declarations;
the probe does **not** duplicate table contents or fabricate addresses:

| Real scanner table | Original address | C++ declaration |
| --- | --- | --- |
| `yy_accept` | `0x08074f40` | `unsigned short[]` |
| `yy_base` | `0x08075540` | **`short[]`** |
| `yy_def` | `0x08075660` | `short[]` |
| `yy_chk` | `0x080759a0` | `short[]` |
| `yy_nxt` | `0x08075780` | `short[]` |
| `yy_meta` | `0x08075460` | `int[]` |

The function's original access pattern and normal Flex C++ NUL
transition logic account for each table reference: accept-state
bookkeeping, initial NUL character index 1, fallback through
`yy_def`, meta conversion above state 140, `yy_chk` validation,
then `yy_nxt` lookup and the jam-state 140 return.

The original scanner object field offsets are authenticated by
instruction addressing: `yy_c_buf_p` at `+0x34`,
`yy_last_accepting_state` at `+0x44`,
and `yy_last_accepting_cpos` at `+0x48`. The compact class
declaration leaves unrelated fields opaque; these arrays model
existing class-layout gaps, not instruction padding or injected code.
The rest of the original lexer source-unit and linked table ownership
are **not** reconstructed, and no whole-ELF identity is claimed.

## Decisive source-level discovery

The initial ordinary C++ source declared `yy_base` as
`unsigned short[]`. GCC generated the original **161-byte**
function length but reproduced only **158/161** instruction bytes,
with mismatches at offsets **46, 108 and 129**. In all three cases
the compiler used a zero-extend (`movzwl`) instead of the original
sign-extend (`movswl`) for an intermediate `yy_base` index.

Changing the single declaration to **`extern const short yy_base[];`**
restores the three original sign-extensions and matches
**161/161** without modifying any other source operation.

`vuc/probes/evaluate_yy_try_nul_signedness_oct13.py`
checks seven ordinary type alternatives under four compiler/flag
combinations, including both recovered compiler dates. It verifies:

- `yy_base` signed: **161/161**, unchanged length and all relocations.
- `yy_base` unsigned: **158/161**, same three mismatched bytes.
- `yy_def` unsigned: **160/161**.
- `yy_chk` unsigned: **159/161**.
- `yy_nxt` unsigned: **160/161**.
- `yy_accept` signed and `yy_meta` unsigned also reproduce
  **161/161**; machine bytes alone do **not** distinguish those
  two signedness choices.
- Plain `-O2` without `-frename-registers` reduces matching to
  **138/161**. The accepted manifest retains the evidence-backed
  default optimization flag pair.

All source variants are normal C++03; there is **no** inline or
handwritten assembly, original byte injection, artificial NOPs,
fabricated addresses, register constraints, or patching of generated
objects.

This first Flex exact reconstruction advanced the accepted inventory
to **191/298** and **13,305** accepted instruction bytes. The
subsequent recovered `yy_get_previous_state()` brings the combined
milestone to **192/298** and **13,510** bytes, with 15 experimental
and 91 unresolved functions. See
`vuc/docs/2026-10-13-flex-previous-state-byte-exact.md`.
Data and BSS verification remain independent of these function proofs.

## Reproduction

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_yy_try_nul_signedness_oct13.py"

limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  C=/tmp/vuc-binutils215/g++-jan25-as215
  "$C" -O2 -frename-registers -c \
    "$R/src/YyTryNulTrans.cpp" \
    -o /tmp/vuc-binutils215/yy-try-nul-exact.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/yy-try-nul-exact.o \
    --config "$R/config/yy_try_nul_trans_oct13_probe.json"
'

python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
```

The canonical function, initialized-data and BSS acceptance scripts
must all remain green before promoting this candidate. A future
source-unit reconstruction should use the existing
`vuc/src/FlexLexer.cpp` inferred class layout and the authenticated
original scanner table declarations, while ensuring no previously
accepted function is lost.

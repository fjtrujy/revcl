# VUC formatted MLTempString constructor: C1 accepted, C2 unresolved

Date: 2026-10-11. Branch: `vuc`.

## Exact result

The original `MLTempString::MLTempString(char const*, ...)` complete-object
constructor (`_ZN12MLTempStringC1EPKcz`) at `0x08071bb0` is
**110/110 instruction bytes exact** when the source in
`vuc/src/MLTempStringFormatted.cpp` is compiled with historical
GCC 3.3.3 (January 25, 2004) and GNU as 2.15, using
`-O2 -frename-registers -falign-jumps=1`.

The matcher individually resolves **eight ELF32 relocations**, including
the original lazy `MLTempStringData::Get()` singleton guard and object
in BSS and the call to original `VFormatString()` at `0x08071a50`.
A normal `va_list` is passed to `VFormatString()`; no machine-code
literals, inserted instruction padding or hand-edited object bytes
were used. The Get() definition in this source is an inline
provisional reconstruction required for historical GCC to reproduce
the original inlined singleton branch.

## ABI-variant limitation

The same ordinary constructor declaration also causes GCC to emit
`_ZN12MLTempStringC2EPKcz` (base-object constructor). This
**remains 98/110 matching bytes**, with differences in compiler
register allocation around the singleton and constructor arguments.
It is deliberately **not** in the strict acceptance manifest.
A second exploratory source under
`vuc/probes/ml_temp_format_inline.cpp` is preserved as diagnostic
evidence for C2.

The exact C1 entry is accepted independently, following the
existing separate C1/C2 verification methodology. The presence of
a non-exact C2 entry in the compiled candidate object **does not**
prove a complete class/translation-unit reconstruction. Before
producing the complete binary, both constructor variants and
the original historical source-unit organization must match.

## Regression status

The strict gate checks **154/154 original function records**,
**9,254/9,254 instruction bytes**, and **399 resolved relocations**.
Of the 298 original named `.text` function records, **154 EXACT,
9 EXPERIMENTAL and 135 UNRESOLVED** are recorded. All 60 host tests pass.
The aggregate figures apply only to individually accepted original
function records, not the full linked ELF.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/resume-154
```

The **mandatory final acceptance criterion is byte-for-byte identity
of the complete original VUC executable**, including linkage, data,
sections and ELF metadata. It is not achieved yet.

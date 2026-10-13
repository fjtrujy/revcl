# VUC Flex get_previous_state: second byte-exact scanner function

Date: 2026-10-13. Branch `vuc`. Baseline: `d4ed221`.
Original function: `yyFlexLexer::yy_get_previous_state()`,
address `0x080613f0`, **205 instruction bytes**.

## Independently verified reconstruction

- Recovered C++03 source: `vuc/src/YyGetPreviousState.cpp`.
- Accepted function manifest:
  `vuc/config/yy_get_previous_state_oct13_probe.json`.
- Historical toolchain: GCC 3.3.3 prerelease, January 25, 2004,
  and GNU as 2.15, `-O2 -frename-registers`.
- Original-ELF comparison: **205/205 instruction bytes exact**,
  **9/9 original relocations**, no initialized literal data.
- The separate December 6, 2003 GCC 3.3.3 snapshot also produces
  the identical 205/205 instruction bytes. This does not prove which
  prerelease compiled the original binary.

The original seven tables referenced through nine relocations are:

| Scanner table | Original ELF address | C++ source type |
| --- | --- | --- |
| `yy_accept` | `0x08074f40` | `unsigned short[]` |
| `yy_ec` | `0x08075060` | `int[]` |
| `yy_base` | `0x08075540` | **`short[]`** |
| `yy_def` | `0x08075660` | `short[]` |
| `yy_chk` | `0x080759a0` | `short[]` |
| `yy_nxt` | `0x08075780` | `short[]` |
| `yy_meta` | `0x08075460` | `int[]` |

No original table contents have been copied or embedded in this
candidate. Original relocations resolve using the existing ELF symbol
manifest. The scanner's class field accesses match the original
`yytext_ptr` at `+0x04`, `yy_c_buf_p` at `+0x34`,
`yy_start` at `+0x3c`, accepting state at `+0x44`,
and accepting input pointer at `+0x48`. Other
`yyFlexLexer` fields remain opaque in the provisional class
declaration; this does not establish the original translation-unit
boundary or the full class implementation.

## Why exactly seven instruction bytes were missing

The first Flex-generated C++ reconstruction already matched
**198/205** with the correct 205-byte length and all nine relocations.
It used an ordinary direct expression:

```cpp
unsigned char yy_c = 1;
if (*yy_cp)
    yy_c = yy_ec[(unsigned char)*yy_cp];
```

The original disassembly loaded the input byte into EAX **before**
initializing the local NUL character byte. GCC emitted the same two
operations in the opposite order for that initial source. The only
seven differing bytes were offsets **32 through 38**, exactly covering
the two reordered instructions.

The ordinary source-level fix is to name the live input byte:

```cpp
unsigned char yy_ch = *yy_cp;
unsigned char yy_c = 1;
if (yy_ch)
    yy_c = yy_ec[yy_ch];
```

That source produces **205/205** with unchanged compiler flags, table
types, relocations, loop structure and class field offsets. No
assembler directives, register constraints, padding instructions,
binary modifications, or fabricated original addresses are involved.

This is an unusually strong independent consistency check on the
adjacent `yyFlexLexer::yy_try_NUL_trans(int)` function reconstructed
in `vuc/docs/2026-10-13-flex-nul-transition-byte-exact.md`:
changing the signed `yy_base[]` back to `unsigned short[]` regresses
`yy_get_previous_state()` to **202/205** with differences at
offsets **87, 152 and 173**, and `yy_try_NUL_trans()` from
**161/161** to **158/161**. Both original functions require the
signed 16-bit base index, independently supporting that C++ type.

## Bounded negative controls

- `vuc/probes/evaluate_yy_get_previous_state_lifetime_oct13.py`
  confirms the original seven-byte sequencing mismatch, the signed
  `yy_base` effect, and both historical compiler variants. The
  original `-O2` *without* `-frename-registers` matches
  only 194/205.
- `vuc/probes/evaluate_get_next_word_local_position_oct13.py`
  tries four natural local-position/leading-whitespace loops,
  six in-word delimiter guard forms, three update orders,
  and four historical compiler configurations for the unresolved
  135-byte `GetNextWord()`. Some variants reproduce the 135-byte
  length but the best match is only 16/135. Do not promote or
  mistake equal length for source fidelity.
- `vuc/probes/evaluate_value_division_case_order_oct13.py`
  tests all six genuine three-arm lexical orders of
  `Value::operator/()` and five float-vector expression forms
  under historical alignment/control options. The previous best
  178/235 remains unchanged; no new candidate is promoted.

The first exact scanner method alone increased the accepted count
from 190 to 191; this second exact method advances it to
**192/298**, covering **13,510/13,510 accepted instruction bytes**.
The diagnostic ledger is now **192 EXACT / 15 EXPERIMENTAL /
91 UNRESOLVED**. The strict historical compiler gate passes
**192/192** accepted functions (**13,510/13,510** instruction bytes),
the data gate passes **244/244** original initialized-data objects
(**71,536/71,536** bytes), and the BSS declaration gate passes
**12/12** original symbol declarations. The host suite passes
**144** tests. These function-level and object checks do not prove
full ELF byte identity.

## Reproduce

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_yy_get_previous_state_lifetime_oct13.py"
limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  /tmp/vuc-binutils215/g++-jan25-as215 -O2 -frename-registers \
    -c "$R/src/YyGetPreviousState.cpp" \
    -o /tmp/vuc-binutils215/yy-prev-state-exact.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/yy-prev-state-exact.o \
    --config "$R/config/yy_get_previous_state_oct13_probe.json"
'
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
```

A complete reconstruction of the original generated scanner still
requires its other methods, authentic compilation-unit boundaries,
table ownership and linker layout. Keep these function-level proofs
separate from any claim about byte identity of the whole executable.

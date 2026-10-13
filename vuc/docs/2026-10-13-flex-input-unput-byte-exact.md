# VUC: Two byte-exact Flex C++ buffer functions

Date: 2026-10-13; branch `vuc`; baseline commit `bedfe69`.
Historical compiler: GCC 3.3.3 (2004-01-25 prerelease) and GNU as 2.15
inside the existing `revcl-oracle` VM.

## New accepted functions

| Original function | Original address | Instructions | Relocations | Source |
| --- | --- | ---: | --- | --- |
| `yyFlexLexer::yyinput()` | `0x08061620` | **170/170** | One authentic `yy_get_next_buffer` PC-relative call | `vuc/src/YyInput.cpp` |
| `yyFlexLexer::yyunput(int,char*)` | `0x08061570` | **167/167** | One 32-byte original error-string literal | `vuc/src/YyUnput.cpp` |

Both are genuine ordinary C++03 source with no inline assembly, no
binary patching, no fabricated function addresses, no register forcing,
and no instruction padding. No new original tables, data declarations,
or BSS objects are synthesized.

All source references use previously authenticated scanner class-field
offsets and the independently verified vtable from
`vuc/docs/2026-10-12-flexlexer-vtable.md`. In particular, the
indirect method call through vtable offset +0x14 from the vptr is
`yyrestart(std::istream*)`, as required by the actual original vtable:
vtable-entry offset +0x1c relative to the beginning of the table,
whose vptr starts at table+8.

## yyinput(): shared EOF return via genuine switch fallthrough

The original machine code uses a common EOF return address. Its
`yy_get_next_buffer()` result switch handles code 2 by calling
`yyrestart(yyin)`, then **falling through into the case-1 EOF return**.
Compiling two independent `return -1;` arms gave 174/170 bytes in
the closest normal-alignment variant and duplicated the return
register initialization. The original source pattern:

```cpp
case 2:
    yyrestart(yyin);
case 1:
    return -1;
case 0:
    yy_c_buf_p = yytext_ptr + offset;
    break;
```

makes GCC generate exactly the original one-return fallthrough block.
Under `-O2 -frename-registers`, the result is **170/170**, with
the original **one relocation** to
`yyFlexLexer::yy_get_next_buffer()` at `0x080611c0`.
No initialized literal bytes are emitted by this function.

The recovered field offsets, including
`yytext_ptr` at +4,
`yyin` at +0x20,
`yy_current_buffer` at +0x28,
`yy_hold_char` at +0x2c,
`yy_n_chars` at +0x30,
and `yy_c_buf_p` at +0x34,
match the original disassembly. The +0x20 input stream and vtable
restart slot are grounded in the original object/vtable. The rest of
the class layout is provisional and represented as reserved space.

Reproducible controls:
`vuc/probes/evaluate_yyinput_eof_fallthrough_oct13.py`,
including separate case returns, natural shared/explicit fallthrough,
goto-to-common-EOF and shared-result forms, under period flags.
With `-falign-jumps=1`, the correct form has a different
167-byte length, so accepted flags use the authentic default
`-O2 -frename-registers`.

## yyunput(): period-standard Flex buffer sliding logic

The original scanner code saves its held byte into the buffer,
checks whether there is enough room for a pushback character,
and if not, slides the existing buffer bytes right by the available
space using a reverse copy:

```cpp
while (source > yy_current_buffer->yy_ch_buf)
    *--dest = *--source;
```

It adjusts both `yy_cp` and the caller-supplied `yy_bp` by the
copy displacement, updates the scanner/buffer character counts, and
dispatches the authentic virtual
`LexerError("flex scanner push-back overflow")`
when the buffer remains too small.

The 32-byte NUL-terminated error string has been independently
extracted from the original ZIP-backed ELF at address
`0x08074dc0`. The strict relocation literal manifest checks the
original bytes there, rather than just substituting an address.
Under `-O2 -frename-registers` the generated source matches
**167/167** instruction bytes and **32/32** original literal bytes,
with the one literal relocation resolved to the original location.
Without `-frename-registers`, the same-length candidate falls
back to **95/167**, confirming the relevant compiler setting.

The original parameter order and virtual method slot also match
the authenticated C++ scanner ABI. There is no additional PLT call
or external code relocation in this function: its fatal path invokes
the virtual `LexerError` through the class vtable.

## Inventory, gates, remaining work

This batch advances the accepted function count from 192 to **194
of 298 original named functions**. The strict gate now verifies
**13,847/13,847** accepted instruction bytes, up from 13,510.
The ledger contains **194 EXACT, 15 EXPERIMENTAL, 89 UNRESOLVED**.
Initialized data and BSS remain checked by independent gates.
The actual linked whole-ELF identity remains **unverified**; the
accepted per-function source fragments are not a complete original
translation unit.

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  C=/tmp/vuc-binutils215/g++-jan25-as215
  "$C" -O2 -frename-registers -c "$R/src/YyInput.cpp" \
    -o /tmp/vuc-binutils215/yyinput-check.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/yyinput-check.o \
    --config "$R/config/yyinput_reconstruct_oct13_probe.json"
  "$C" -O2 -frename-registers -c "$R/src/YyUnput.cpp" \
    -o /tmp/vuc-binutils215/yyunput-check.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/yyunput-check.o \
    --config "$R/config/yyunput_reconstruct_oct13_probe.json"
'
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
```

`yy_get_next_buffer()` remains a substantial, unresolved
550-byte generated Flex method and is a natural next target,
alongside source-unit unification of the now byte-exact
`yy_get_previous_state`, `yy_try_NUL_trans`, `yyunput`,
and `yyinput` methods. Reusing the recovered original vtable
layout, buffer state, and signed scanner tables is preferable
to unrelated compiler-flag brute forcing.

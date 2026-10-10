# VUC byte-exact lexer stream and destructor ABI reconstruction

Date: 2026-10-10. Branch: `vuc`.

## Strict acceptance

Recovered January 25, 2004 GCC 3.3.3 and GNU as 2.15 now reproduce
**119/119 accepted original function-symbol records**, **6,323/6,323
instruction bytes**, **267 checked ELF32 relocations**, and 35
referenced original literal bytes. All 50 host tests pass. The
original ELF in its entirety **does not yet match byte for byte**.

| Additional original function | Original address | Exact bytes | Relocations |
| --- | --- | ---: | ---: |
| `yyFlexLexer::switch_streams(std::istream*,std::ostream*)` | `0x080610d0` | 96/96 | 0 |
| `yyFlexLexer::LexerInput(char*,int)` | `0x08061130` | 97/97 | 1 |
| `yyFlexLexer::~yyFlexLexer()` (D2) | `0x08060fe0` | 78/78 | 6 |
| `yyFlexLexer::~yyFlexLexer()` (D1) | `0x08061030` | 78/78 | 6 |
| `yyFlexLexer::~yyFlexLexer()` (D0) | `0x08061080` | 78/78 | 7 |

These five functions contribute **427 new byte-exact instruction
bytes** and **20 verified relocations**.

## Reconstructed Flex scanner behavior

The `switch_streams()` implementation follows the original
virtual-table call sequence: if a replacement input stream is
non-null, delete the current buffer (original vtable slot `+0x10`),
create a new `0x4000`-character input buffer (slot `+0x0c`),
then switch to the new buffer (slot `+0x08`). Independently,
a non-null output-stream argument replaces the output pointer at
object offset `+0x24`. The current-buffer field resides at
`+0x28`.

`LexerInput()` is a one-character C++ stream read. Its original
inline stream-state checks are not equivalent to a single
`istream::good()` call, and reproducing that distinction was
necessary to match the original generated instructions:
check `eof() || fail()` before reading; invoke
`istream::get(buffer[0])`; return zero on EOF after the
read; otherwise return `-1` on a bad stream and `+1` on success.
The original call target is `std::istream::get(char&)` at
`0x080496ec`.

The three virtual destructor entrypoints come from an ordinary
C++ derived-class destructor. It deletes the scanner state
at offset `+0x4c`, calls `yy_delete_buffer()` on the current
buffer, and runs the `FlexLexer` base destructor. The original
`yyFlexLexer` and `FlexLexer` vtable targets, `operator
delete`, the buffer-deletion method, and `_Unwind_Resume`
relocations are all verified against the canonical ELF. GCC emits
the expected D2, D1 and deleting D0 variants, including exception
unwind. The provisional source class is an ABI-level view;
the original headers and translation-unit boundaries are not
yet authenticated.

## Open investigations

The original `yyFlexLexer::yy_create_buffer()` is still only
131/139 bytes in the closest tested source-level hypothesis.
`yy_delete_buffer()` remains 85/87 and
`yy_load_buffer_state()` has an unmatched
callee-saved-register allocation. They **must not enter the
accepted source or strict manifest** until all bytes agree.

The source-language constructs above are ordinary historical C++.
They contain no fabricated machine-code blobs, inline assembly
to force an output instruction, or manual alignment padding.

The canonical inventory contains **119 EXACT, 8 EXPERIMENTAL,
171 UNRESOLVED** named original function records out of 298.
This is a symbol ledger, not a binary-wide equality percentage.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-119
```

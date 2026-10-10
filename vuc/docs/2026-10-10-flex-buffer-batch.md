# VUC Flex lexer buffer reconstruction and local relocation correction

Date: 2026-10-10. Branch: `vuc`.

This checkpoint continues the byte-for-byte VUC 2004.1 reconstruction.
No whole-executable byte identity is claimed.

## Exact source functions

The strict historical GCC 3.3.3 / GNU as 2.15 gate now verifies
**97/97 named original function records**, **4,553/4,553 instruction
bytes**, **201 resolved relocations**, and **32 literal bytes** verified
within their original referenced sections. It passes all 50 host
unit tests. In the original 298-symbol text-function inventory,
**97 are EXACT, 8 EXPERIMENTAL, and 193 UNRESOLVED**.

Two new routines are accepted in `vuc/src/FlexLexer.cpp`:

- `yyFlexLexer::yy_init_buffer(yy_buffer_state*, std::istream*)`
  at `0x080618a0`: **53/53 exact bytes**, one relocation.
  It calls the original flush helper and initializes the input
  stream, interactive state and input buffer flags.
- `yyFlexLexer::yy_flush_buffer(yy_buffer_state*)`
  at `0x080618e0`: **80/80 exact bytes**, one relocation.
  It resets character count, null-terminates the first two sentinel
  positions in the buffer using the lexer's buffer index, resets
  the position/fill/line flags, and reloads state when flushing
  the currently active buffer.

The original `yy_buffer_state` stores the character-buffer pointer
at offset `+4`, the buffer cursor at `+8`, current character count
at `+0x10`, and initialization flags at `+0x18`, `+0x1c`,
`+0x20`, and `+0x24`. The lexer object stores current buffer at
`+0x28`, the buffer index at `+0x6c`, and its input stream
at `+0x20`. The exact generated code and original member offsets
are evidence; the provisional `reserved` layout fields are **not**
authenticated original declarations.

## Relocation correction

The strict matcher previously interpreted the `st_value` of a
defined local `STT_FUNC` as an addition to an explicitly supplied
original linked address. That is wrong for a concrete function name:
the defined function's `st_value` is its relative offset in the
candidate object and must **not** be added a second time. The
`linked_symbol_offset()` helper in `match_probe.py` now ignores
such candidate-local code offsets while continuing to resolve
section-relative object addends and the original targets strictly.
A new host unit test checks the defined-function case against
section-symbol and undefined-function handling.

This is particularly important when `yy_init_buffer()` calls its
sibling `yy_flush_buffer()` in the **same candidate object**. The
full exact regression gate remains green after the correction.

## Investigated, but not accepted

- `yyFlexLexer::yy_load_buffer_state()` at `0x08061780`:
  the original is 39 bytes and saves/restores `%ebx`, whereas
  several natural C++ forms produce a different 37-byte register
  allocation. Other evaluated forms produce 39 bytes but do
  not match the original instruction sequence. Its source
  hypotheses remain in `vuc/probes/`.
- `yyFlexLexer::yy_delete_buffer()` remains **85/87** bytes at
  best, differing in two instruction-register encodings. The
  original owner flag, buffer storage and `free()` behavior are
  understood, but it is not in the exact manifest.

## Reproduction

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-resume-97
```

Any mismatched instruction byte, missing relocation target,
incorrect original symbol or literal is a failure. **Full ELF
byte-exact identity remains mandatory and unachieved.**

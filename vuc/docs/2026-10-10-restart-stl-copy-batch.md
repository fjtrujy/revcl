# VUC byte-exact checkpoint: lexer restart and historical STL copy helpers

Date: 2026-10-10. Branch: `vuc`.

## Exact reconstructed functions

The strict relocation-aware gate now validates **100/100 original function
symbol records**, **4,811/4,811 original instruction bytes**, **203
resolved relocations** and 32 literal bytes, after compiling with the
January 25, 2004 GCC 3.3.3 source snapshot and GNU as 2.15. The 50
host tests pass. This is **not** complete-executable identity.

| Original function | Address | Checked bytes |
| --- | --- | --- |
| `yyFlexLexer::yyrestart(std::istream*)` | `0x080616d0` | 93/93 |
| `std::__uninitialized_copy_aux<ConstTypeIterator,Type*>()` | `0x0804f0e0` | 79/79 |
| `std::__uninitialized_copy_aux<TypeIterator,TypeIterator>()` | `0x0804f1c0` | 86/86 |

The lexer restart implementation matches the canonical function's behavior:
if there is no current buffer, obtain a 16-KiB buffer from the virtual
`yy_create_buffer()` method (the original virtual-table offset is
`+0x0c`), select it, call the reconstructed original
`yy_init_buffer()` and tail-call `yy_load_buffer_state()`. The
unreconstructed virtual-method declarations in
`vuc/src/FlexLexerRestart.cpp` retain a provisional source layout.
When the method was compiled together with other reconstructed lexer
methods, its body differed in two register bytes, whereas a standalone
ordinary C++ source fragment produced the original 93 bytes. This
documents a remaining translation-unit/register-allocation
integration problem; it is not evidence for the historical source
partition.

Both `Type` STL instantiations come directly from the era's
libstdc++ `<bits/stl_uninitialized.h>` implementation. The source
reconstructs the non-POD `Type` 16-byte layout with a four-word
copy constructor. Explicit instantiation produces the two original
symbol names and their **79/79** and **86/86** byte bodies, including
ordinary iterator increments, placement construction and the original
return conventions. No fabricated machine code is involved.

## Still experimental

Two `AsmVariable` STL instantiations were also generated with the
same historical header, and their **67-byte** and **74-byte**
lengths agree with the original; their copy-constructor word order
and register choices do not. The original 8-byte
`AsmVariable` representation is plausible, but these routines
must stay outside the exact manifest.

`yy_create_buffer()` follows the original dual-`malloc`,
buffer-ownership and virtual-error paths, and currently reaches
**131/139** exact instruction bytes with five mapped relocations
and its 44-byte error literal verified. GCC 3.3.5, multiple original-
date optimization switches, and multiple ordinary source forms still
emit those eight nonmatching bytes; it remains experimental.

The existing `yy_delete_buffer()` (85/87) and `Printer_UglyPrint()`
(38/40) are also explicitly non-exact. The full linker and original
Debian GCC 3.3.3 patchset are not authenticated.

The original ELF text-symbol inventory is now **100 EXACT**, **8
EXPERIMENTAL**, **190 UNRESOLVED** out of **298**. Those are record
counts only. The mandatory end condition is complete executable
byte-for-byte equality, which has **not been reached**.

## Reproduce

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-100
```

# Byte-exact VUC yyFlexLexer constructor reconstruction

Date: 2026-10-12. Branch: `vuc`.

## Independently verified original ELF functions

The recovered source-date GCC 3.3.3 (2004-01-25 prerelease) and GNU as
2.15 produce the original C++ constructor entries directly from
`vuc/src/FlexLexerConstructors.cpp` with `-O2 -frename-registers`:

| Original symbol | Original address | Verified code | Relocations |
| --- | --- | --- | --- |
| `yyFlexLexer::yyFlexLexer(std::istream*,std::ostream*)` C2 | `0x08060ec0` | **138/138 bytes** | 1 |
| `yyFlexLexer::yyFlexLexer(std::istream*,std::ostream*)` C1 | `0x08060f50` | **138/138 bytes** | 1 |

The candidate is an ordinary, full C++ constructor body. It does not use
handwritten assembly, fake instruction bytes, padding instructions, or
original ELF offsets embedded in executable machine-code arrays.
Both ELF32 relocations target the original named `_ZTV11yyFlexLexer`
vtable at `0x08074c80`, with the normal C++ vtable address-point
addend of eight bytes. The complete C1/C2 symbols and all 276 instruction
bytes are verified by the strict matcher.

The original disassembly constrains the constructor's initializer
stores. Input and output stream pointers are at object offsets
`0x20` and `0x24`. The current buffer pointer is at `0x28`.
The constructor also initializes the mode/condition fields at
`0x0c`, `0x10`, `0x14`, `0x18`, `0x1c`, `0x34`,
`0x38`, `0x3c`, `0x40`, `0x4c`, `0x64`, `0x68`,
`0x6c`, `0x70` and `0x74`.

Crucially, expressing those assignments in the *original observed
order* reproduces GCC's original load/store schedule, including
the apparently nonmonotonic `0x74` before `0x70` and `0x18`
before `0x14` initialization. The field offsets are grounded in
the original executable. Provisional C++ field names and the full
historical lexer type declaration are **not authenticated**.

## Acceptance and remaining restrictions

These are two original named function-symbol records, not two distinct
algorithms: the Itanium ABI emits both C1/C2 constructor variants.
Both are added separately to the strict acceptance manifest.
The historical compiler and original ELF are kept unchanged.

The full gate now verifies **165/165** accepted original function
records and **11,283/11,283** instruction bytes. Exactly two
new records add **276** checked bytes compared with the 163-record
checkpoint. The full linked VUC executable, code outside accepted
records, data, metadata, layout and linking are **not** byte-exact.

Reproduce using the existing Linux VM:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/oct12-gate-165
```

The regression suite uses the canonical ZIP member for comparison,
resolves the vtable relocation against the original ELF symbol, and
rejects any candidate body that differs by even one instruction byte.
No whole-executable score is inferred from this function-level milestone.

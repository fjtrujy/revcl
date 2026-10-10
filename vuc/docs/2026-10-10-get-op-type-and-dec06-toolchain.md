# VUC type inference and December 2003 GCC toolchain recovery

Date: 2026-10-10. Branch: `vuc`.

## Exact `GetOpType(IR::Expr_Data*)`

The original function at `0x0805a4f0` is now reproduced using normal
historical C++: **117/117 original instruction bytes** and **2/2
resolved relocations** with GCC 3.3.3 (20040125 prerelease), GNU as
2.15, `-O2 -frename-registers`.

Its input is an IR expression with a kind word at byte offset 4 and an
operator pointer at byte offset 8. When the expression kind is 5,
there is a sequence of ten tests of the nested operator kind, in the
exact original order **11, 13, 12, 10, 7, 6, 5, 4, 3, 2**. For these
cases the function returns the operator's type pointer at offset 8.
Otherwise it creates an `IR::UInt_Data` initialized with kind 1.
The allocated object uses the original `IR::UInt_Data` vtable address
point `0x08072520`, which is `_ZTVN2IR9UInt_DataE + 8`, and calls
the original `operator new(unsigned)` PLT entry at `0x080499ec`.

Writing those ten cases as individual early `if` returns is essential.
An apparently equivalent grouped `switch` lets GCC replace several
compares with ranges and emits **77 rather than 117 bytes**. The
recovered `vuc/src/GetOpType.cpp` has no inline assembly, artificial
padding or emitted byte arrays. Its classes are *provisional ABI
reconstructions*, not proven original header declarations. The vtable
and RTTI data emitted by the candidate translation unit still require
whole-binary data and linker-layout verification.

## Historical compiler comparison

The original ELF `.comment` contains both January 25, 2004 and
December 6, 2003 GCC 3.3.3 prerelease strings, so we reconstructed
an **additional source-date compiler** from public GCC source
revision `82bacd9e1bfbbb138c1401ec85d4a7dd597836d5`, which says
`GCC 3.3.3 20031206 (prerelease)`.

`vuc/scripts/bootstrap_dec06_gcc.sh` pins its upstream source tarball
SHA256 `519d85092d67b6814b21de47adbff98469e22152440c39423574c6736323efb4`
and the original Bison 1.875d package SHA256
`93a3264eed76aa85455cf6385fcad5f161112ad17535b0b52be889906d94df41`.
The source-date GCC `cc1plus` initially failed to link because its
pre-generated `cfns.h` uses old GNU89 inline semantics: the current
host compiler's default semantics omitted `libc_name_p`.
Building scratch host compiler objects with `-fgnu89-inline` and
running `make stmp-int-hdrs` to populate `stddef.h` and `stdarg.h`
produces a usable `cc1plus`/`xgcc` combination.

In the Linux VM the successful scratch build lives at
`/tmp/vuc-gcc333-dec06`; the compatible C++ standard headers are from
an external historical GCC 3.3.5 package, **not proven original Debian
headers**. A scratch wrapper selecting GNU as 2.15 was created by
adapting the existing January wrapper to use the December GCC binary.
The December compiler was then tested against four original functions:

| Function | December exact instruction bytes | Status |
| --- | ---: | --- |
| `__tcf_1` Instruction arena cleanup | 45/46 | Unaccepted |
| `Canonize2_Reduce()` | 83/85 | Unaccepted |
| `Printer_UglyPrint()` | 38/40 | Unaccepted |
| `yy_delete_buffer()` | 85/87 | Unaccepted |

These scores are unchanged from the source-date January compiler and
show that the 2003 source date alone does **not** explain their
remaining register assignments. **This is not the original Debian
GCC 3.3.3 `0pre3` compiler**; original patches, headers, link flags,
assembler, startup objects and linker scripts are still unresolved.

## Exact gate and limitations

After promoting `GetOpType()`, the strict accepted set is now
**133/133 original function-symbol records, 6,761/6,761 instruction
bytes, 289 resolved ELF32 relocations, 36 verified literal bytes**.
All 51 host tests pass. The original 298-entry ledger contains
**133 EXACT, 10 EXPERIMENTAL and 155 UNRESOLVED** records. These are
function-symbol counts only; no whole-binary percentage is claimed.
**Complete VUC ELF byte-for-byte equality remains mandatory and
unachieved**.

Reproduction:

```sh
sh -n vuc/scripts/bootstrap_dec06_gcc.sh
limactl shell revcl-oracle -- sh vuc/scripts/bootstrap_dec06_gcc.sh \
  /tmp/vuc-gcc333-dec06 2
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/gate-133
```

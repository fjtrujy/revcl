# VUC byte-exact bitwise operators and Value ABI evidence

Date: 2026-10-11. Branch: `vuc`.

## Accepted reconstructions

Two previously unresolved C++ operators now match the authenticated
VUC 2004.1 ELF **byte for byte**, compiled with January 25, 2004
GCC 3.3.3 and GNU as 2.15:

| Original symbol | Address | Exact original instruction bytes |
| --- | --- | --- |
| `Value::operator&(Value const&) const` | `0x0806f5d0` | **143/143** |
| `Value::operator|(Value const&) const` | `0x0806f660` | **143/143** |

The accepted `vuc/src/ValueBitwise.cpp` uses ordinary C++ to
construct an integer-typed result, call
`Value::TypeConvertForce(TypeID, Value const&, Value const&, Value&, Value&)`
and conditionally apply `&=` or `|=` to the result's integer bits.
Both calls resolve to the original `0x0806f8b0` target.
No instruction bytes, artificial branches or assembly padding
are inserted.

The critical code-generation clue was the declaration order of two
locals. The first hypothesis declared `Value temporary` before
`Value result` and matched only **87/143** original code bytes.
Declaring the result *first* (and the temporary second) yielded
**143/143** for both operators under the same historical
`-O2 -frename-registers` flags. The two source statements are
semantically equivalent but trigger different zero-initialization
scheduling in GCC 3.3.

The provisional ABI layout uses 32-bit `TypeID`-compatible signed
`type` at offset 0, two further 32-bit fields, four consecutive
byte flags at offsets 12-15, and the integer payload starting
at offset 16. Choosing `int type` (rather than unsigned)
preserves both exact bitwise bodies and agrees with the signed
branch pattern independently observed in the neighboring
arithmetic methods. The broader original `Value` class layout,
header organization and translation-unit boundaries remain
provisional.

## Verification

The strict original-symbol gate now passes **152/152 named
function-symbol records**, **8,923/8,923 instruction bytes**,
**390 resolved original ELF32 relocations** and
**132 separately checked literal bytes**. The historical
298-symbol queue is **152 EXACT, 10 EXPERIMENTAL,
136 UNRESOLVED**. All 60 host tests pass.

These are per-symbol results. There is **no whole-executable
byte equality**, and no function-count ratio is presented
as an ELF-wide matching percentage. Complete ELF identity
remains the mandatory final acceptance condition.

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-152
```

## Neighboring arithmetic investigation

The same ordinary C++ class is now being used to reconstruct
arithmetic operations with a signed type discriminator, a
four-element float payload and a historical
`Value::TypeConvert` call. An initial subtraction hypothesis
matches **221/221** bytes under `-falign-jumps=1` and is
subject to independent source promotion and full-gate
verification. The addition hypothesis is currently 216 bytes
versus 219 original and stays experimental. Neither
arithmetic function is counted in this checkpoint.

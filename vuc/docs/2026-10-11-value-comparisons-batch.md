# VUC six byte-exact Value comparison operators

Date: 2026-10-11. Branch: `vuc`.

## Original code recovered

The original VUC 2004.1 ELF has six adjacent Value comparison
functions. All six now reconstruct **byte for byte**, without assembly,
fabricated padding, or manually inserted machine-code sequences:

| Original function | Original address | Exact instruction bytes |
| --- | --- | ---: |
| `Value::operator==` | `0x0806ed40` | 254/254 |
| `Value::operator!=` | `0x0806ee40` | 254/254 |
| `Value::operator>` | `0x0806ef40` | 254/254 |
| `Value::operator<` | `0x0806f040` | 254/254 |
| `Value::operator>=` | `0x0806f140` | 254/254 |
| `Value::operator<=` | `0x0806f240` | 254/254 |

They share the original 32-byte `Value` layout and
`TypeConvert(Value const&, Value const&, Value&, Value&)` call target
`0x0806f7d0`. Every original call relocation is verified. Each
method constructs a boolean-typed `Value`, applies type conversion,
and handles integer types 1/2, scalar float type 3 and four-component
float type 4. The original integer branches for relational comparisons
use signed conditions (`setg`, `setl`, etc.), so the comparison
payload is modeled as signed `int` rather than `unsigned`.

The source is consolidated in **one ordinary C++ translation-unit
candidate** `vuc/src/ValueComparison.cpp`, which compiles all six
exact functions together. The original complete class declaration
and precise source file membership are not yet authenticated.

## Historical compiler-code-generation discovery

Earlier `-O2 -frename-registers -falign-jumps=1` equality/inequality
probes generated 259 bytes instead of the canonical 254. GCC emitted
`fucompp` with extra unordered/NaN-aware parity-flag branches.
The original VUC code instead uses the classic x87
`flds` / `fcomps` / `fnstsw` / `sahf` instruction sequence.

Testing actual historical GCC options established that

```
-O2 -frename-registers -ffast-math
```

without forcing jump alignment reproduces the original 254-byte
control flow, including its original jump-target alignment, for
both equality and inequality. The same flags and signed-integer
type reproduce **all four relational operators**. All 1,524
instructions-as-bytes are authenticated via the same original-symbol
relocation-aware matcher used by the strict acceptance suite.

`-ffast-math` changes floating-point behavior, particularly
NaN/unordered comparisons. In this case the original executable's
actual x87 instruction bytes, not an assumption about source-level
IEEE semantics, determine the required compiler behavior. These
flags are a successful source-era reproduction recipe; they do
**not** authenticate the original Debian GCC patchset or its exact
compiler command line.

A focused flag sweep is preserved as
`vuc/probes/evaluate_value_float_flags.py`. With the same source,
`-ffast-math -falign-jumps=1` changes the function length to
245 bytes; default jump alignment is essential.

## Strict gate and nonmatches

This batch adds **six distinct original function-symbol records and
1,524 new verified original instruction bytes**. The full strict
acceptance gate is expected to pass **162/162 records, 10,927/10,927
bytes, 409 resolved relocations**, with 132 verified literal bytes.
The original 298-symbol inventory becomes **162 EXACT,
9 EXPERIMENTAL and 127 UNRESOLVED**.

Other `Value` arithmetic methods (notably multiplication, addition
and division) remain non-exact. Applying `-ffast-math` to those
candidates did not independently produce whole-function equality;
the existing source probes and incomplete outcomes remain
experimental.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-162
```

**Complete ELF identity is still not achieved.** Exact acceptance
of individual functions is necessary but insufficient: original
remaining functions, data, object/section layout, linker output,
padding, ABI runtime and ELF metadata must eventually match the
canonical executable byte for byte.

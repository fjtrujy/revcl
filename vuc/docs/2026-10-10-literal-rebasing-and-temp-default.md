# VUC byte-exact MLTempString default constructor and strict literal rebasing

Date: 2026-10-10. Branch: `vuc`.

## Accepted function

The historical GCC 3.3.3 (Jan 25 2004) plus GNU as 2.15
reconstructs the **complete C1 ABI constructor**
`MLTempString::MLTempString()` at `0x08071c90`:
**102/102 executable bytes**, **nine linked relocations**, and the
original single-byte empty C-string literal. The ordinary C++
implementation in `vuc/src/MLTempStringDefault.cpp` calls the
inline singleton `MLTempStringData::Get()` and
`FormatString("")`. Reproducing the historical singleton
initializer and its one-time `memset` yields the original
GCC-generated fast and cold paths. The original guard and
`tsd` static objects are verified against canonical ELF symbols
at `0x0808bce8` and `0x0808c520`.

The C2 ABI sibling at `0x08071c20` is separately modeled and
compiles to the original **101-byte length**, but differs at six
register/operand bytes (95/101). It remains EXPERIMENTAL,
not byte-exact and not accepted. The two variants are not
interchangeable simply because they share constructor source
semantics.

## New relocation-literal validation

The original ELF sometimes links multiple string literals emitted
together in one candidate object section to distinct, noncontiguous
original addresses. A single `.rodata` base would be incorrect.
`vuc/scripts/match_probe.py` now supports an explicit
`relocation_literals` mapping of candidate section byte ranges
to the corresponding **individual original ELF addresses**.
Acceptance requires:

- Every byte of the candidate literal section to belong to
  **exactly one** mapped range.
- Every mapped range to be within the candidate section and
  the original `.rodata` section.
- Exact byte identity with the original ELF for **every byte**,
  including null terminators.
- Each literal relocation to resolve through exactly one
  verified mapped range.
- No simultaneous whole-section and per-range mapping.

Unsupported relocations, gaps, overlaps, incorrect bytes,
incorrect original addresses or missing targets produce errors;
this cannot turn non-exact output into an accepted result by
ignoring a mismatched literal. A new unit test exercises all
those conditions with three concatenated literals.

This enabled an independent check of
`Print(ASTFunCall*, std::ostream&)` at `0x0805b770`.
Its three source literals `"("`, `", "` and `")"`
are verified at distinct original addresses
`0x08075c5d`, `0x080730a6` and `0x08072803`,
seven literal bytes in total, and all seven call/data
relocations. The function body is currently **96/98**
matching bytes with GCC `-O2`; the remaining two
differ in callee-address register selection, so the
function remains **EXPERIMENTAL**.

## Exact gate and open work

The strict gate now passes **120/120 explicitly accepted
original function records**, **6,425/6,425 instruction
bytes**, **276 fully resolved relocations**, and **36
per-probe checked literal bytes**. The host suite passes
**51 tests**. The exhaustive 298-function ledger has
**120 EXACT, 10 EXPERIMENTAL, and 168 UNRESOLVED**
records.

The complete ELF is **not yet byte identical**; the original
Debian-patched compiler, full linker order, all data sections,
remaining function bodies and ELF metadata are still under
reconstruction. The mandatory goal remains exact **whole-file**
byte equality, not a byte-match percentage over an accepted subset.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-120
```

# Fresh VUC ConcatenateBinOp reconstruction: 223/346

Date: 2026-10-15. Original ELF VUC 2004.1 Linux i386, target
`ConcatenateBinOp(IR::Operator_Data*, const MLGameString&, const MLGameString&)`
at `0x080507c0`, 346 bytes. This was **UNRESOLVED** with no
previous tracked C++03 candidate at the start of this batch.

## Source recovery

The original assembler dispatches on the 32-bit operator field at
`IR::Operator_Data + 4` via comparisons for 13, 12, 11, 10, 7,
6, 5, 4, 3, 2, 1, 0, 9, 8. It selects an operator string, then:

1. calls `rhs.c_str()`, then `lhs.c_str()` (the historical argument
   evaluation order in the call expression);
2. constructs an `MLTempString` with format string `"(%s%s%s)"`;
3. constructs the returned `MLGameString` from that temporary.

Unsupported operation codes retain an **empty operator string**, an
original-era source quirk rather than an error return.

The new experimental source `vuc/probes/ConcatenateBinOpNewOct15o.cpp`
reconstructs the normal C++03 if/else dispatch and uses the real
already-recovered MLGameString and MLTempString class API. No invented
functions, inline assembly or handcrafted instruction bytes.

## Authenticated strings and relocations

The candidate compiler object emits 44 bytes of `.rodata` containing
16 source literals. All are checked against original linked ELF bytes,
including substring sharing in the original string pool:

| Text | Original address |
| --- | --- |
| empty | `0x08072286` |
| `||` | `0x0807248e` |
| `|` | `0x0807248f` (points inside original `||`) |
| `(%s%s%s)` | `0x08072491` |
| `&&` | `0x0807249a` |
| `&` | `0x0807249b` (points inside original `&&`) |
| `<=`, `>=`, `<`, `>`, `!=`, `==` | original `0x0807249d..0x080724aa` |
| `/`, `*`, `-`, `+` | original `0x080724ad..0x080724b3` |

The strict verifier normalizes 16 original literal/section addresses
and 4 call sites (two `MLGameString::c_str`, one
`MLTempString` constructor, one `MLGameString` constructor):
**20/20 authentic relocation sites, 44/44 literal bytes**.

Jan25 and Dec06 GCC 3.3.3 prerelease snapshots with GNU as 2.15,
compiling the same genuine C++03 source, both give **223/346
original-offset instruction bytes**, full 346-byte original size,
20 relocations and 44 literal bytes. The chosen alignment is the
historically plausible `-O2 -frename-registers -falign-jumps=4
-falign-labels=2 -falign-loops=1`. A bounded source/branch
placement experiment found an alternative original-sized 346-byte
candidate with jump=8/labels=1, but it matched only 142/346;
the original-sized jump=4/labels=2 candidate is decisively better.
The remaining differences, starting at original offsets +37, +46,
+55 and later aligned labels, are compiler branch displacement/
block alignment. A complete exact reconstruction has **not** been
established.

The config `vuc/config/concatenate_binop_partial_oct15o.json` is
selected solely in the experimental manifest, never the accepted exact
manifest. Generated candidate objects are retained under
`/tmp/vuc-binutils215/concat-original-alignment-oct15o/` inside
the existing Lima guest.

## Next

Recover the original translation-unit context for string operation
serialization or another previously untouched expression binding
function before sweeping more labels or branch options.
The three newly recovered partials in this batch (two ReduceBinOp
specializations and ConcatenateBinOp) change the 298-function status
to **204 EXACT, 30 EXPERIMENTAL, 64 UNRESOLVED**.

The full original ELF is not byte-identically rebuilt.

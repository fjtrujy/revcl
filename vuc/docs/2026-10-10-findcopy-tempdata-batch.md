# VUC function batch: FindCopy, temporary data and dispatch

Date: 2026-10-10. Branch: `vuc`.

## Byte-exact outcome

The exact-only acceptance set grows from **25/25 to 30/30 individually
verified function-symbol entries**. All **1,362 of 1,362 accepted instruction
bytes** match the canonical ZIP-backed VUC ELF after resolving **41 ELF32/i386
relocations**. The accepted sources also verify 29 per-manifest read-only
literal bytes. A passing function set **does not mean that the complete ELF
has been reproduced**, and the final requirement remains whole-file
byte-for-byte equality.

The five new verified functions are:

| Function | Original start | Original size | Exact bytes | Verified relocations |
| --- | --- | ---: | ---: | ---: |
| `MLHashTable<char const*, MLGameStringData*, ..., 513>::FindCopy()` | `0x08071e20` | 114 | 114 | 2 |
| `MLTempStringData::Get()` | `0x080719d0` | 77 | 77 | 7 |
| `SymbolTable::EndScope()` | `0x0806a500` | 27 | 27 | 1 |
| `yyFlexLexer::LexerOutput()` | `0x080611a0` | 18 | 18 | 1 |
| `Value::Set(ASTExpr const*)` | `0x0806e7a0` | 42 | 42 | 2 |

All implementations are normal C++ and are compiled using the previously
recovered GCC 3.3.3 20040125 compiler and GNU as 2.15. The relocations
resolve to original symbols, with no hardcoded instruction or NOP bytes.

## 1. FindCopy: hash-table node layout and independent temporary hashers

The original linked-list node is 16 bytes on i386, in this order:

| Field | Byte offset |
| --- | ---: |
| Key, `char const*` | 0 |
| Cached unsigned hash | 4 |
| `MLGameStringData*` payload | 8 |
| Next node pointer | 12 |

The hash-table contains 513 bucket pointers; GCC emits division by this
compile-time constant as a multiplication by `0xff803fe1` followed by
shifts/subtractions. The reconstructed algorithm calculates the hash,
selects the bucket, compares each node's cached hash, and invokes
`MLHashFun<char const*>::IsEqual()` to confirm a collision before
returning the node payload. Failure returns a null payload.

A crucial source reconstruction detail is that **two different hasher
temporaries** exist: `Hasher().Hash(key)` and
`Hasher().IsEqual(node->key,key)`. Reusing a single local `Hasher`
produces the right source-level behavior but different stack slots and
register assignments, and only matches **88/114 bytes**. Independent
temporaries reproduce **114/114 bytes**, including both relative calls.

The shared `MLHashTable.h` owns the node layout; `MLHashTable.cpp`
owns the already-accepted `clear()` instantiations. The provisional
`MLHashTableFindCopy.cpp` source compiles its accepted specialization
separately, because combining emitted template methods in the partially
reconstructed historical translation-unit context changes the historical
register allocation (81/114). This is a **temporary reconstruction
boundary**, not a claim that it was originally a separate source file.
We must consolidate original TU grouping before complete ELF rebuilding.

## 2. Temporary-string getter: rebase *named* BSS objects

The original ELF retains two local object names and original addresses:

| Original symbol | Address | Size |
| --- | --- | ---: |
| `_ZGVZN16MLTempStringData3GetEvE3tsd` | `0x0808bce8` | 8 |
| `_ZZN16MLTempStringData3GetEvE3tsd` | `0x0808c520` | `0x2fc4` |

`Get()` returns a statically initialized `MLTempStringData` object
named `tsd`. Its constructor fills a `0x2fc0`-byte buffer with ASCII
`'X'` (`0x58`), terminates `bytes_[0x1fbf]`, and zeros the trailing
cursor field. The original 77-byte function follows GCC's pre-thread-safe
static-local initialization pattern. This class layout and bytecode
reconstruct without injecting assembly.

Initially, only **69/77 bytes** compared equal. The remaining eight
differences were solely in absolute addresses: our current relocatable
object places the guard and data contiguously, whereas the historical
linker laid them out separately. Mapping the entire temporary `.bss`
section to one absolute base is incorrect.

The new `relocation_objects` manifest field allows `.bss` references
to be resolved via the exact object symbol and its original address.
For `STT_SECTION` relocations, it finds the unique containing candidate
`STT_OBJECT` and computes the original symbol address plus the
within-object offset. For direct `STT_OBJECT` and `SHN_COMMON`
relocations, it uses the explicit original object address. Every mapped
name/address/section is independently validated against the **original
ELF's own `.symtab`**. Ambiguous and unmapped objects fail rather than
defaulting to guessed addresses.

With these checks, the ordinary C++ implementation passes **77/77
bytes** and seven verified relocations. Unit tests cover exact rebasing,
unknown objects, ambiguous ranges, and incorrect original addresses.

The `MLTempStringData.h` header now provides one shared C++ class layout
for `Get()` and `FormatString()`, avoiding contradictory definitions
in accepted source fragments.

## 3. Additional cross-subsystem functions

`SymbolTable::EndScope()` matches all 27 instruction bytes. The
scope pointer sits at offset 4 in the reconstructed `SymbolTable`,
and subtracting one `MLHashTable` instance advances by `0x804` bytes
(`513 * 4`), before a tail call to the matching specialization's
`clear()` at `0x0804f020`. The pointer-size arithmetic is derived
from the recovered template layout.

`yyFlexLexer::LexerOutput()` matches all 18 bytes. Its output stream
pointer is at offset `0x24`, and the function performs a tail call to
`std::ostream::write` at the original PLT destination `0x0804999c`.

`Value::Set(ASTExpr const*)` matches all 42 bytes. The expression-kind
field is at offset 8; kinds 2 and 3 dispatch by tail calls to the
`ASTConstValExpr` and `ASTLValExpr` overloads. The historical
`-falign-jumps=1` setting is necessary to avoid twelve bytes of
modern/source-default branch alignment padding. Both previously accepted
`Value` constructor variants remain exact under their original
regression flags.

## Still experimental

The original `MLGameStringTable::Get()` reconstruction remains
non-exact (up to 94/106 bytes under tested semantically plausible
initializer variants), despite a separately matching 22-byte generated
cleanup callback. `AsmOutput_WritePrologue()` remains 22/30 exact code
bytes. Neither is in the accepted source tree.

The formatted `MLTempString(char const*, ...)` **C1** entry matches
**110/110 bytes** when the static `MLTempStringData::Get()` definition
is visible for inlining. The same ordinary C++ source presently produces
only **98/110 bytes** for the **C2** entry; those entries require original
constructor/codegen ordering investigation. Because the candidate source
emits both, it stays under `vuc/probes/` and *neither* entry enters the
accepted manifest until the emitted entries are fully verified.

`Printer_UglyPrint()` also has the correct 40-byte body length but
currently matches 38/40 bytes. It stays experimental. The predicate
`IsFoldable()`, default temporary-string constructors,
`MLTempString::Set()`, and the longer `VFormatString()` and string
table `Add()` remain to be reconstructed.

## Repeat the verified work

With the existing Lima VM and historical compiler/assembler wrappers:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-30

python3 vuc/scripts/coverage.py --write
python3 -m unittest discover -s vuc/tests -v
```

The complete status ledger contains **30 EXACT, 5 EXPERIMENTAL,
263 UNRESOLVED** of the 298 original text-function symbols. As with all
previous batches, that is a symbol accounting, **not** a complete-binary
byte match or percentage. The original upstream archives and mainline
VCL reconstruction remain untouched.

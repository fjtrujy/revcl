# VUC semantic-analysis list reconstruction batch

## Verified accepted functions

Using the historical GCC 3.3.3 (20040125) and GNU as 2.15 compiler/assembler
oracle, four more VUC functions are **byte-exact** against the canonical
ZIP-backed ELF:

| Original symbol | Address | Exact code bytes | Relocations |
| --- | --- | ---: | ---: |
| `Semant_Check(ASTModule*)` | `0x0806a080` | 40/40 | 2 |
| `Check(ASTNodeList<ASTVarDecl*>&)` | `0x0806a0e0` | 45/45 | 1 |
| `Check(ASTNodeList<ASTStmt*>&)` | `0x0806a160` | 45/45 | 1 |
| `Check(ASTNodeList<ASTDecl*>&)` | `0x0806a190` | 45/45 | 1 |

**34/34 accepted functions, 1,537/1,537 original instruction bytes, 46
independently resolved relocations.** Only accepted named functions are
counted, not the VUC executable as a whole.

## Recovered structure

The original `Semant_Check` references a variable-declaration list at
offset `0x14` and declaration list at `0x20` of the AST module. These
offsets are consistent with a **12-byte `ASTNodeList<T>`** on i386:

```cpp
template <class T> struct ASTNodeList {
    int length;      // offset 0
    int capacity;    // offset 4 (provisional field name)
    T *entries;      // offset 8
};
```

The field names are hypotheses, not recovered original identifiers.
The `Check` specialization body loops across `length`, loads a pointer
from the entries array, increments the index, and invokes the overloaded
`Check(ASTDecl*)` / `Check(ASTStmt*)` function at the original resolved
address. All three specializations have identical instruction structure
except their ELF32 PC-relative relocation targets. An ordinary C++
template and explicit instantiations reproduce every byte.

The flag set `-O2 -frename-registers -falign-jumps=1` removes
extra instruction-padding differences while preserving normal C++
instructions. The shorter `Semant_Check()` dispatcher requires only
`-O2 -frename-registers` and makes two tail/call references exact.

Accepted source lives in `vuc/src/ASTNodeList.h`,
`vuc/src/CheckLists.cpp` and `vuc/src/SemantCheck.cpp`.
Those fragments do **not** constitute the complete historical translation
units. They must eventually be reconciled with actual original source
boundary evidence.

## Exploration that remains unresolved

A separate analysis of `yyFlexLexer::yy_load_buffer_state()` recovered
the field-offset pattern and a normal C++ body of 37 bytes, versus the
original 39. The original saves/restores the `%ebx` register; GCC
currently selects registers differently, with no tested ordinary
optimization flag resolving the discrepancy. It stays under
`vuc/probes/flex_load_buffer.cpp`, excluded from the strict gate.

The `MLTempString(char const*,...)` constructor C1 remains exactly matched
in its experimental source, but C2 differs; the `Printer_UglyPrint`
hypothesis remains 38/40; `MLGameStringTable::Get()` remains non-exact.
No partial function is promoted to accepted source.

## Reproduce

```sh
python3 vuc/scripts/coverage.py --write
python3 -m unittest discover -s vuc/tests -q

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-34
```

The accepted function gate must exit zero with 34 exact, 1537 matching
instruction bytes. The exhaustive original symbol inventory has
**34 EXACT, 5 EXPERIMENTAL, 259 UNRESOLVED** of 298 text function
symbols; it is not a whole-program matching percentage.

Historical original compiler patches, complete original source,
data/linker layout, and byte-for-byte reproduction of the whole
original ELF are still outstanding.

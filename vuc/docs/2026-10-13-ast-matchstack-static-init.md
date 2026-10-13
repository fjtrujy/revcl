# Byte-exact VUC AST.cpp startup and matchStack reconstruction

Date: 2026-10-13, branch `vuc`.

## New strict function matches

Original ELF32 names and addresses were read from the immutable, SHA256-verified
VUC 2004.1 ZIP member. The January 25, 2004 GCC 3.3.3 compiler wrapper
and historical GNU assembler 2.15 now generate:

| Original function | Address | Instruction bytes | Relocations |
| --- | --- | ---: | ---: |
| `__static_initialization_and_destruction_0(int,int)` (AST.cpp) | `0x0804a720` | **145/145** | 11 |
| `_GLOBAL__I_matchStack` (AST.cpp) | `0x0804a7d0` | **31/31** | 1 |

Both use **one ordinary C++ compilation unit**, `vuc/src/AST.cpp`, and
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`.
The 14 literal bytes of the original string `"_return_value"` at
`0x08072244` are independently verified in the initializer comparison.
The helper maps its BSS and linked references through named original
symbols, not manufactured addresses:

- `std::__ioinit` at `0x0808abe0`;
- `matchStack` at `0x0808abe4` (12 bytes);
- `ASTReturnStmt::regName` at `0x0808abf0` (4 bytes);
- the original `__tcf_0` at `0x0804a7c0`;
- `std::ios_base::Init::Init()`, `__cxa_atexit`, `__dso_handle`, and
  `MLGameString::MLGameString(char const*,bool)`.

The **16/16 byte** original `__tcf_0` also matches when built in this
same source unit, with both relocations resolved. That callback was
**already exact**; it is not counted as an additional function and its
existing accepted manifest remains mapped to the independent
`vuc/src/IostreamInit.cpp` source for generator compatibility.

## Key source-level discovery

Original BSS symbols, initialization order, and code show that
`matchStack` is a 12-byte object with three pointer-width zeroing
stores during dynamic global construction. A raw `char[12]` definition
is not enough to reconstruct the original 145-byte initialization
helper, although the compiler can coincidentally generate a matching
31-byte global wrapper from it. Conversely,
`std::vector<int> matchStack` adds a destructor registration and
produces a different generated helper (and a 29/31 wrapper).

The accepted source uses an ordinary three-pointer container with
an inline zeroing constructor and *no destructor*. This reproduces
the complete original helper's instruction schedule and all its
relocations; it does not prove the precise original class or template
spelling. Its actual `begin`, `end`, and `capacity` field semantics
remain a type-reconstruction hypothesis until callers are recovered.

The following natural-source variants remain separately preserved:

- `vuc/probes/ast_return_register_init_oct13.cpp`: raw 12-byte array;
- `vuc/probes/ast_matchstack_vector_oct13.cpp`: standard vector with
  incorrect extra destructor registration;
- `vuc/probes/ast_matchstack_custom_oct13.cpp`: the original matching
  source-level model prior to production integration.

None of these experiments introduces handwritten assembly, machine
bytes, padding objects, invented linker mappings, or modifications to
the original binary.

## Reproduction and coverage

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-ast-170
```

The strict manifest now accepts **170/170 named original function records**
and **11,561/11,561 instruction bytes**. The original 298-function
inventory is **170 EXACT**, **7 EXPERIMENTAL**, **121 UNRESOLVED**.
The host regression suite has 120 passing tests. The candidate source
unit is not yet a complete reconstructed program; the **whole ELF has
not been rebuilt byte-identically** and the exact Debian compiler/
linker provenance is still incomplete.

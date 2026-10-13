# VUC startup and source-unit context: useful negative controls

Date: 2026-10-13; branch `vuc`.

These probes use normal C++ in existing original-era source fragments and
compile with the recovered GCC 3.3.3 / GNU as 2.15 toolchain. They are
**not** exact-function promotions and must not be confused with the
separate strict accepted manifest.

## AST.cpp construction forms

The original AST.cpp startup has a 12-byte BSS `matchStack`, a 4-byte
`ASTReturnStmt::regName`, and the `_return_value` literal. All are
original ELF-derived and linked at authenticated original addresses.

| Source representation | Generated helper | `_GLOBAL__I_matchStack` |
| --- | --- | --- |
| Plain `char matchStack[12]` plus named return global | 111 bytes (original 145) | 31/31 exact |
| `std::vector<int> matchStack` plus named return global | Extra vector destructor registration | 29/31 partial |
| Native three-pointer container with inline zeroing constructor | **145/145 exact** | **31/31 exact** |

This directly demonstrates why matching a generated 31-byte wrapper alone
is not sufficient to authenticate the original C++ source or source unit.
All three sources are retained in `vuc/probes/ast_*_oct13.cpp` and the
accepted three-pointer model is in `vuc/src/AST.cpp`.

## currFileName with a genuine entry-point neighbor

`vuc/probes/curr_file_with_main_oct13.cpp` combines the *already exact*
`vuc/src/CurrFileInitialization.cpp` with the *already exact*
`vuc/src/Main.cpp` using ordinary source inclusion. Under
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`:

- The existing original `__static_initialization_and_destruction_0ii`
  at `0x0806f9f0` remains **88/88** byte-exact with six relocations.
- The original `_GLOBAL__I_currFileName` at `0x08071580` remains
  **29/31**, with the same two register-choice differences as the
  isolated unit; adding a plausible real entry-point neighbor is not
  sufficient to reproduce original register-renaming context.

This is **not** an accepted additional function and does not prove the
historical original `vuc_compiler.cpp` compilation unit. Revisit only
with new authentic source-order/callee evidence.

## Broader Canonize2 source-neighbor control

`vuc/probes/canonize_with_genuine_neighbors_oct13.cpp` additionally
includes real IR predicate/global-environment source alongside the
conversion and canonization functions. Its constructor helper is
**185/193**, even though the canonicalizer is **85/85** and the
five adjacent application functions plus global wrapper and cleanup
callbacks remain exact under the combined optimizer flags. The
accepted narrower `vuc/src/ConvVal.cpp` source does not regress that
initializer: the original strict environment manifest retains its
independent exact source. This is a functional, not cosmetic,
original-code ownership boundary to resolve before a whole-ELF build.

The compiler comparison outputs are disposable VM scratch `.o` files;
the files in `vuc/probes` and the archived original ELF remain
unchanged except for the newly authored versioned hypothesis sources.

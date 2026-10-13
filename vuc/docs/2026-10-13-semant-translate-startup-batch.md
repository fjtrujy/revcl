# VUC semantic and IR translation source-unit startup reconstruction

Date: 2026-10-13. Branch: `vuc`.

## Two newly accepted compiler-generated original function records

| Original function | Address | Exact instruction bytes | Resolved relocations |
| --- | --- | ---: | ---: |
| `_GLOBAL__I__Z20CheckActualParamListRK11ASTNodeListIP7ASTExprERKSt6vectorI4TypeSaIS6_EERK12MLGameString` | `0x0806a0c0` | **31/31** | 1 |
| `_GLOBAL__I__Z9TranslateRK4Typeji` | `0x0806e050` | **31/31** | 1 |

Both matches come from ordinary C++ in `vuc/src/SemantUnit.cpp` and
`vuc/src/TranslateTypeUnit.cpp`, using the recovered GCC 3.3.3 compiler
(January 25, 2004) and GNU assembler 2.15 with
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`.
The original ELF32 relocations point respectively to the real
`__static_initialization_and_destruction_0(int,int)` functions at
`0x080619d0` and `0x0806a960`. No original ELF or archive bytes have
been rewritten. No assembly, injected instructions, artificial padding,
linker address manipulation, or fabricated predecessor functions were used.

## Semantic analyzer context

The Semant.cpp function order recovered from the original symbol table
starts at `CheckActualParamList` (`0x08061a20`), followed by
`Check(ASTFunCall*)` (`0x080623f0`), several other Check overloads, and
`Semant_Check(ASTModule*)` (`0x0806a080`).

An ordinary C++ source fragment implements a real argument-count and
argument-type checking loop and then the previously reconstructed
`Check(ASTFunCall*)` and `Semant_Check` methods. In the *same compiled
object*, both previously accepted methods remain byte-exact:

- `Check(ASTFunCall*)`: **118/118**, all three relocations verified.
- `Semant_Check(ASTModule*)`: **40/40**, both relocations verified.

The provisional `CheckActualParamList` emits **210 bytes** while the
original is **2,510 bytes**. Its original diagnostic policy and full
type-coercion rules are unknown; the method is **not** accepted. It is
a genuine source-level reconstruction hypothesis, not a fake helper
used to produce a good-looking wrapper. The initial experiment is
preserved in `vuc/probes/semant_real_unit_first_oct13.cpp`; an alternate
ordering remains in `vuc/probes/semant_real_unit_oct13.cpp`.

## Type translation context

The original Translate.cpp unit begins with
`Translate(Type const&, unsigned int, int)` at `0x0806a9b0`
(352 bytes), followed by IR translation helpers, with the startup
wrapper at `0x0806e050`. The original disassembly branches on type
flags masked with `~0x3c00`, constructs constant or register IR data,
and returns a polymorphic node pointer. The source-level hypothesis
in `vuc/src/TranslateTypeUnit.cpp` models that genuine dispatch with
ordinary C++ types and allocations. It produces **246 bytes** for
the underlying translator versus 352 original bytes; the missing
IR node variants and original type semantics are not yet recovered,
so the application body is **not** byte-exact or accepted.

The compiler nevertheless emits the exact original **31/31** startup
function keyed to `Translate(Type const&, unsigned int, int)`, including
its relocation. Its independent working probe is preserved as
`vuc/probes/translate_real_unit_oct13.cpp`.

## Strict acceptance and remaining scope

The accepted candidate manifest grows from **175** to **177** original
function records, **11,832/11,832** instruction bytes. Nine original
functions remain experimental and 112 unresolved out of 298 records.
The standalone accepted source fragments will need consolidation into
a real historical translation-unit/link structure; neither these
wrappers nor the other accepted symbols demonstrate complete linked
ELF byte equality.

Reproduction:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-semant-translate-177
```

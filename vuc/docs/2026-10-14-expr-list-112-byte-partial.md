# VUC ASTExpr list: 112/256 verified after pointer-ownership sweep

Date: 2026-10-14. Branch: `vuc`.
Source baseline: `e151548`. Original authority: checksum-verified
archived VUC 2004.1 ELF32 i386, symbol `0x0806ae20`.
Build oracle: original-era **GCC 3.3.3 20040125 prerelease**,
GNU **as 2.15**, in the existing Lima `revcl-oracle` guest.

## New independently verified partial improvement

The experimental `Translate(ASTNodeList<ASTExpr*>&)` reconstruction
improves from **90/256** to **112/256 authenticated original
instruction bytes** (**+22 matching bytes**). It is **not exact**.

Candidate: `vuc/probes/translate_expr_list_partial_improved_oct14.cpp`.
Updated existing selected experimental manifest:
`vuc/config/translate_expr_list_oct14_probe.json` with flags

```sh
-O2 -frename-registers -falign-jumps=4 -fno-reorder-blocks
```

The checker confirms:
- **256-byte** emitted function, equal to the original function size.
- **112/256** identical instruction bytes at original offsets;
  `exact: false`.
- **2/2** authenticated original relocations: the
  `Translate(ASTExpr*, unsigned, int)` call at `0x0806bf00`
  and `operator new[](unsigned)` at `0x080499ac`.
- No extra call or fabricated relocation, no handwritten assembly,
  no source-level padding or instruction patching.
- Same i386 hidden-return-pointer result ABI and observable
  three-word `ASTNodeList` result layout.

A local variable `old_size = length` is retained across the list
capacity check, and the input index's old value is saved separately
before incrementing. Those choices give GCC more historically
compatible temporaries and allocation/copy branch layout than the
previous ordinary `Add(Translate(input.entries[i++]))` form.

**Important limitation:** the original VUC code uses a guarded,
alignment-sensitive `rep movsl` copy of its previous pointer array.
The 112-byte experimental source still uses an ordinary scalar loop.
Its semantics, exact original translation-unit ownership, exception
behavior under allocation failure, or compatibility with every
possible list state are not guaranteed by the byte score. The original
prologue's index register and several stack-slot offsets still differ.
It would be incorrect to promote this to `vuc/src/`.

## Reproducible experiment matrix

The first bounded driver,
`vuc/probes/evaluate_ast_expr_copy_lifetimes_oct14.py`, explored
seven ordinary or compiler-intrinsic pointer-copy paths, four
capacity/data ownership variants, seven different index/call
lifetimes, and four historical GCC optimization/alignment modes.
That is **784 attempted builds**, of which **38** have the
original 256-byte length and meet the original relocation contract.
The best was **112/256** with
`copy-loop-local-length--postinc-explicit`, `nrj4`.

A second bounded driver,
`vuc/probes/evaluate_ast_expr_index_and_append_oct14.py`, started
from this new source and tested 13 index/call control flows, six
list-append ownership forms, and five historical options:
**390 attempted builds**. The best remained **112/256**.
Some invalid lexical forms were rejected at compilation and are not
claimed as matches. The `copy-local-ptr` C++03 scope was then
corrected; a targeted run added **65 valid compiler builds**, none
with the original function size.

Total: **1,239 attempted historical compiler configurations**.
Only original-length candidates with authenticated relocations
were scored. No exact match was found.

## Rebuild and strict comparison

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- "$C" \
  -O2 -frename-registers -falign-jumps=4 -fno-reorder-blocks \
  -c "$R/probes/translate_expr_list_partial_improved_oct14.cpp" \
  -o /tmp/vuc-binutils215/ast-expr-improved-independent-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/ast-expr-improved-independent-oct14.o \
  --config "$R/config/translate_expr_list_oct14_probe.json"
```

The saved candidate was compiled and verified **independently of the
matrix's generated files**, so transient compiler output is not the
only evidence. The old 90/256 source remains available as a
historical, deliberately unselected experiment.

## Scope and next leads

All existing accepted functions and their source are unchanged.
The function inventory stays **197 EXACT / 19 EXPERIMENTAL /
82 UNRESOLVED**. The exact acceptance manifest remains unchanged;
this is a **partial-function gain**, not a new exact function or a
verified whole-linked-ELF match.

The next useful move is to recover the genuine original `Add`
definition and its copy-alignment control flow, which currently
generates extra branch/stack movement. Focus especially on the
original `+0xa7` tail-copy pre-alignment check,
`rep movsl`, and how old length/capacity are passed to the
inlined allocator; repeated superficial spelling and alignment
flags are unlikely to close the remaining 144 bytes.

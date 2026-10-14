# VUC fragment-list partial checkpoint — 2026-10-14

Baseline: `vuc` at `7b23c3d`. The original VUC 2004.1
ELF32 i386 remains the authority. This batch does **not** claim a newly
byte-exact function or improvement to existing byte-match percentages.

## Two newly tracked experimental functions

The previously reconstructed `TranslateFragList` specializations are
now first-class, reproducible **experimental** candidates:

| Original symbol | Address | Original length | Exact instruction bytes at offset | Resolved external relocations |
| --- | --- | ---: | ---: | ---: |
| `TranslateFragList<ASTVarDecl*>` | `0x0806e0a0` | 240 | **41/240** | 2/2 |
| `TranslateFragList<ASTDecl*>` | `0x0806e190` | 240 | **41/240** | 2/2 |

Both original call sites are authenticated: `TranslateFrag(ASTDecl*)`
at `0x0806d2c0`, and `operator new[](unsigned)` at
`0x080499ac`. The original compiled function size and these relocations
are required by `match_probe.py`; neither routine is byte-exact.

Saved source:

- `vuc/probes/translate_frag_list_var_partial_oct14.cpp`
- `vuc/probes/translate_frag_list_decl_partial_oct14.cpp`

Their existing probe manifests were updated to reference those
standalone, identical-source-family candidate files and the matching
`-O2 -frename-registers -falign-jumps=4` compiler flags.
These names were placed in `selected_candidates.json`, **not** in
`exact_candidates.json`, and neither was promoted to `vuc/src/`.

The basic source implements list-result zero initialization,
`TranslateFrag` calls, 16-element initial capacity and dynamic doubling,
and old-entry copying through `__builtin_memcpy`. The source uses a
pointer iterator, which compiles to the original length but diverges
strongly in register lifetime and loop order. The 41-byte count should
not be construed as a complete decompilation or original source
provenance. We have **no newly exact instruction bytes** in the
accepted inventory.

The existing `evaluate_translate_frag_list_oct14.py` was rerun
against the historical January 2004 GCC 3.3.3 / GNU as 2.15 setup.
Its **210** variants cover two C++ instantiations, seven legitimate
list-append implementations, three outer-loop shapes and five compiler
modes. Both selected 41/240 forms were recompiled independently from
their saved source files and verified with the strict original ELF
matcher. No attempt produced an exact routine.

## Separate structural negative finding

The preceding investigation into the AST expression-list translator
showed that ordinary `__builtin_memcpy` under period GCC 3.3.3
does generate the original `cld; rep movsl` machine idiom.
However, the tested 249-byte body is seven bytes shorter than the
256-byte target; adding padding to engineer a match would be invalid.
The selected 112/256 experimental translator remains unchanged.

## Inventory, guardrails and next work

After registering the two formerly unresolved routines, status is
**198 EXACT / 21 EXPERIMENTAL / 79 UNRESOLVED** out of 298 named
original functions. The newly tracked entries represent improved
*coverage bookkeeping* (two original-sized real partials), **not**
an increase in byte-exact accepted functions.

Future work should recover the actual original
`ASTNodeList<IR::Fragment_Data*>::Add()` inlining and loop-index
storage, test genuine source-unit predecessor definitions, and compare
the original `rep movsl` alignment path. The current 41/240
baseline is especially weak around the function prologue.

Historical acceptance requires the previous **198 exact functions
(14,501 bytes)** plus independent checks of **244 initialized-data
objects (71,536 bytes)** and **12 BSS declarations**. A complete
linked-ELF byte match remains unestablished. The VCL root and original
archives, other branches, and all 163 pre-existing untracked paths
were left untouched.

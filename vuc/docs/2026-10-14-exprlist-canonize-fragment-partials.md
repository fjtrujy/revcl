# VUC unresolved ASTExpr translation and fragment reduction

Date: 2026-10-14. Branch: `vuc-unresolved-oct14`.
Baseline HEAD: `f0ba14a`. Historical oracle: canonical VUC 2004.1
Linux ELF in the original ZIP; recovered January 25, 2004 GCC 3.3.3
prerelease and GNU as 2.15. The original binary, archive, and VCL
root reconstruction have not been modified.

## Result

Neither function reached authenticated byte-exact status. Two new
reproducible, original-length partials are preserved in `vuc/probes/`
with dedicated strict relocation-aware manifests:

| Function | Original ELF address | Original size | Best match | Verified relocation targets |
| --- | --- | ---: | ---: | ---: |
| `Translate(ASTNodeList<ASTExpr*>&)` | `0x0806ae20` | 256 | **90/256** | 2 |
| `Canonize_Reduce(IR::Fragment_Data*)` | `0x08050460` | 224 | **78/224** | 11 |

These numbers are within each function, not new accepted instruction-byte
coverage. Both original-length partials have `exact: false`.
The acceptance manifest and `vuc/src/` remain unchanged.

## 1. Expression list translation: ABI and semantic graph

The original returns `ASTNodeList<IR::Expr_Data*>` through an i386 hidden
return pointer and an ABI return of `ret $4`. The list has three 32-bit
fields: `length` at +0, `capacity` at +4, `entries` at +8. Each starts
at zero. It walks the input list and invokes
`Translate(ASTExpr *, unsigned int, int)` with exactly **0x3c00** and
**0** at original VA `0x0806bf00`. For each returned expression pointer,
it extends a result array; initial capacity is **16**, subsequent capacity
doubles, with four bytes per element. An inline copied-array path uses
`rep movsl`; allocation is the genuine `operator new[](unsigned)` at
`0x080499ac`.

The ordinary historical C++ source at
`vuc/probes/translate_expr_list_oct14.cpp` implements this exact
high-level call graph; compiler output has **244 bytes**, not 256.
A `-fno-reorder-blocks` baseline places the loop at original relative
`+0x40`, but other lifetime/register decisions still differ and the
length is usually 251 bytes.

`vuc/probes/evaluate_translate_expr_list_oct14.py` tests independently
compiled source variants for old-pointer ownership, store order, capacity
lifetime, old element count and `for`/`while`/postincrement loops,
combined with period GCC 3.3.3 `-falign-jumps` and block ordering.
The original-length best is **90/256** under
`-O2 -frename-registers -falign-jumps=4 -fno-reorder-blocks`.
It comes from an ordinary pointer-copy loop and a postincrement input
index rather than the original inlined `rep movsl`, so **it is diagnostic
rather than a claim about the original authored source**. A verified copy
of that 256-byte source is saved as
`vuc/probes/translate_expr_list_partial_oct14.cpp`; the manifest is
`vuc/config/translate_expr_list_oct14_probe.json`.

The remaining mismatches include different stack homes for the input
index and saved array pointer, as well as copy-loop register allocation.
The source-level `memcpy` path should be revisited against original GCC
RTL before trying further alignment tricks.

## 2. Fragment canonicalization: genuine constructor lifetime

The original 224-byte
`Canonize_Reduce(IR::Fragment_Data*)` has the following branching:

- Kind 1: allocate a **16-byte hash node**, calculate
  `MLHashFun<MLGameString>::Hash` on `fragment+8`, copy the key
  `MLGameString`, store the hash at node+4, copy the mapped
  `MLGameString` from `fragment+12` into node+8, then insert into
  one of **513** global environment buckets with node+12 linking to
  the previous bucket head. The original uses the 32-bit multiply
  reciprocal for modulo 513.
- Kind 2: check `fragment+24` for a non-null statement, clear the
  global environment and call `Canonize_Reduce(IR::Stmt_Data*)`.
- Otherwise, return without taking action.

The initial structural source
`vuc/probes/canonize_fragment_oct14.cpp` uses manual placement
constructors and emits 210 bytes. This has the right fields and
symbol calls but not the original cleanup lifetime. A more faithful
ordinary `new FragmentHashNode(name, output, Hash(name))` with a
C++03 node constructor, saved in
`vuc/probes/canonize_fragment_ctor_args_oct14.cpp`, emits **222
bytes**. Crucially, the actual historical compiler now emits a
constructor-throw cleanup via `operator delete` and
`_Unwind_Resume`, matching the original's exceptional control flow.

`vuc/probes/evaluate_canonize_fragment_oct14.py` explores semantic
node ownership, argument order, ordinary switch and early-return
paths, with historical alignment controls.
`vuc/probes/evaluate_canonize_ctor_args_oct14.py` extends this to
genuine constructor/argument lifetimes, original kind-2-first dispatch,
bucket lookup forms, original compiler alignment and branch frequencies.

A 224-byte experimental case matches **78/224** with all eleven checked
relocation sites (eight distinct relocation targets) and is preserved at
`vuc/probes/canonize_fragment_partial_oct14.cpp`; its manifest is
`vuc/config/canonize_fragment_oct14_probe.json`. This source tests
`__builtin_expect(fragment->kind == 2, 0)` and uses a
`globalEnv.heads + slot` pointer owner. Historical GCC 3.3.3 supports
both, but their presence in original C++ has **not** been authenticated;
they must not be promoted to production on a mere length match.

The first diagnostic mismatches appear around +0x1a and +0x20.
Remaining divergences in the node-construction path include saved
pointer register selection, positioning of the hash call, and node
link/store sequencing. Importantly, the normal C++ node-constructor
candidate provides much better source-level justification than manual
placement construction, even though it is currently two bytes short.

## Reproduction

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_translate_expr_list_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_canonize_fragment_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_canonize_ctor_args_oct14.py"

limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
  -falign-jumps=4 -fno-reorder-blocks \
  -c "$R/probes/translate_expr_list_partial_oct14.cpp" \
  -o /tmp/vuc-binutils215/translate-expr-saved-partial-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/translate-expr-saved-partial-oct14.o \
  --config "$R/config/translate_expr_list_oct14_probe.json"

limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
  -falign-jumps=2 -falign-labels=1 -falign-loops=1 \
  -c "$R/probes/canonize_fragment_partial_oct14.cpp" \
  -o /tmp/vuc-binutils215/canonize-fragment-saved-partial-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/canonize-fragment-saved-partial-oct14.o \
  --config "$R/config/canonize_fragment_oct14_probe.json"
```

All generated compiler variants live in `/tmp/vuc-binutils215` inside
the existing Linux VM, not in tracked source. No inline assembly,
hand-authored jump bytes, fabricated machine NOPs, byte patching or
invented relocation targets are used.

## Next work

The strongest next moves are to recover actual `ASTNodeList::Add()`
source and ownership for the two list pipelines, and authentic node
constructor/emission provenance for the `Canonize2.cpp` translation unit.
The VCL compiler's GCC 3.2.2 findings are hypotheses only; this VUC
work uses the recovered GCC 3.3.3 toolchain throughout.

No additional exact function has been accepted; inventory remains
**195 EXACT / 15 EXPERIMENTAL / 88 UNRESOLVED** until strict matching
authorizes a change. Whole-ELF equality has not been established.

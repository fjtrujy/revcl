# VUC: AST fragment-list translation improves from 41 to 110 bytes

Date: 2026-10-14. Working branch: `vuc`. Baseline:
`6278378`. Original target: unmodified VUC 2004.1 Linux i386
ELF, with its original named symbol/relocation table as the authority.

## New source-level progress

The two already tracked experimental function candidates improve
substantially and independently, with **no false exact promotion**:

| Original function | Original address | Original size | Before | After | Improvement | Real relocations |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `TranslateFragList<ASTVarDecl*>` | `0x0806e0a0` | 240 | 41/240 | **110/240** | +69 | 2/2 |
| `TranslateFragList<ASTDecl*>` | `0x0806e190` | 240 | 41/240 | **110/240** | +69 | 2/2 |

Across the two original-sized functions, this is **138 additional
identical instruction bytes at their original offsets**. It does *not*
add new fully authenticated functions to the exact inventory.

The two original call relocation targets are authenticated:
`TranslateFrag(ASTDecl*)` at `0x0806d2c0` and
`operator new[](unsigned)` at `0x080499ac`. Every new candidate
rebuilds as exactly **240 bytes** and passes the strict matcher
relocation policy, returning `exact: false`.

Updated selected *experimental* source:

- `vuc/probes/translate_frag_list_var_partial_improved_oct14.cpp`
- `vuc/probes/translate_frag_list_decl_partial_improved_oct14.cpp`

The previous 41/240 sources remain available as historical
experiments in `translate_frag_list_{var,decl}_partial_oct14.cpp`.

The manifests `vuc/config/translate_frag_list_{var,decl}_oct14_probe.json`
now use:

```sh
-O2 -frename-registers -falign-jumps=4 -fno-reorder-blocks
```

The choice of C++03 source was transferred from the independently
improved 112/256 AST expression-list partial. In particular, the
new source uses a named `old_size = length` before its append
capacity check, a saved `old_entries` pointer, and a separate
`index=i; ++i;` before passing the indexed AST node to
`TranslateFrag`. This matches the original fragment-list prologue,
loop-entry address and several allocator/branch regions better than
the previously selected pointer-iterator source.

**Limits of the claim:** The original function copies previous
pointer-array storage using alignment-sensitive `cld; rep movsl`.
The presently higher-scoring C++03 source uses a scalar element-copy
loop. Their exact code generation is different and equivalence under
all exceptional/allocation conditions is not established. The
original starts by zeroing EDX, while the candidate zeroes EAX,
and some stack slots and the original append/register lifetimes
still differ. The score is a *partial object-code match*, not
authentic source authorship or a byte-for-byte ELF match.

## Reproducible targeted search

`vuc/probes/evaluate_fraglist_ast_expr_transfer_oct14.py`
uses the real template instantiations, the historical GCC 3.3.3
(2004-01-25 prerelease) and GNU as 2.15, original relocation
map, and six independently selectable source dimensions:

- seven ordinary append-copy forms, including scalar loop,
  saved-size scalar, `__builtin_memcpy`, `__builtin_memmove`
  and local destination/source variants;
- seven real caller index/call lifetimes (`for`, `while`,
  separate index, postincrement, named result and pointer iteration);
- both original target template instantiations;
- eight historic GCC `-O2` / register-renaming / jump-alignment /
  block-order modes.

This is **784 attempted compiler configurations**. Some attempted
source forms are rejected by the original C++ compiler and are
not given a matching score. **32** generated function objects match
the original 240-byte length and have authenticated relocation maps.
The maximum is **110/240**; no exact match was produced.

The two chosen sources were recompiled **independently of the sweep**
and verified at 110/240 with **both recovered GCC 3.3.3 snapshots**
(January 25, 2004 and December 6, 2003), with matching original
relocations each time. All generated scratch sources/objects are kept
outside the tracked project under
`/tmp/vuc-binutils215/fraglist-ast-expr-transfer-corrected-oct14`
inside the existing Lima guest.

### Strict reproduction

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
for kind in var decl; do
    limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
      -falign-jumps=4 -fno-reorder-blocks \
      -c "$R/probes/translate_frag_list_${kind}_partial_improved_oct14.cpp" \
      -o "/tmp/vuc-binutils215/fraglist-${kind}-independent.o"
    limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
      --object "/tmp/vuc-binutils215/fraglist-${kind}-independent.o" \
      --config "$R/config/translate_frag_list_${kind}_oct14_probe.json"
done
```

Each reports `length: 240`, `exact_bytes: 110`,
`relocations: 2`, `exact: false`.

## Inventory and next work

The VUC ledger stays **198 EXACT / 21 EXPERIMENTAL /
79 UNRESOLVED**. The known 198 accepted exact functions
total 14,501 original instruction bytes, and are not changed by
this batch. The two updated manifest entries remain selected
experimental candidates; neither is listed in
`vuc/config/exact_candidates.json`.

For further semantic reconstruction, recover the authentic
`ASTNodeList::Add()` construction/copy sequence and its real
translation-unit predecessor context. A stronger future candidate
must preserve both the original pointer-copy idiom and the
original 240-byte function length rather than introduce artificial
padding or inline assembly.

The original VUC and VCL archives, existing worktree, and all
pre-existing untracked paths are preserved. Full linked-ELF byte
identity remains unestablished.

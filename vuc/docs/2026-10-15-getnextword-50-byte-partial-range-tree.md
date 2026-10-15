# VUC 2004.1 — authentic 50/135 GetNextWord C++03 partial

**Date:** 2026-10-15. **Repository:** `/path/to/revcl`.
**Branch:** `vuc`. **Original authority:** unchanged archived VUC 2004.1
Linux i386 ELF32. **Historical compilers:** the January 25, 2004 and
December 6, 2003 GCC 3.3.3 snapshots with GNU assembler 2.15 under the
existing `revcl-oracle` Lima guest.

## Initial checks and preservation

The tracked worktree was clean at start on `vuc`, initial HEAD
`983f96a`, with **155 unrelated untracked paths**. Successfully ran
`git fetch origin && git rebase origin/main` before touching the
source. All 155 unrelated files were preserved. No new worktrees,
subagents, original-binary modifications, root VCL source edits,
manual machine instructions, forced registers or fake padding.

## Positive result: GetNextWord improved 23 → 50 matched bytes

| Property | Prior selected | Improved selected |
| --- | ---: | ---: |
| Original function | `_Z11GetNextWordPKc` | Same |
| Original ELF virtual address | `0x080511e0` | Same |
| Original function length | 135 | 135 |
| Compiled historical GCC function length | **135** | **135** |
| Equal original-offset instruction bytes | **23/135** | **50/135 (+27)** |
| Independently original-resolved relocation sites | **5/5** | **5/5** |
| Data symbols with relocation sites | 3 | 3 |
| Exact-match acceptance | **NO** | **NO** |
| Coverage classification | EXPERIMENTAL | **EXPERIMENTAL** |
| Historical compiler snapshots | January and December | **January and December** |

The updated selected code is
`vuc/probes/GetNextWordNestedRangePartialOct15f.cpp`. The existing
experimental config path
`vuc/config/get_next_word_partial_oct15e_probe.json`
is deliberately reused rather than adding a duplicate manifest
entry; the source and flags are updated, and the previous
`vuc/probes/GetNextWordByteBoundsPartialOct15e.cpp` is retained as an
unmodified historical diagnostic.

The improved source's compiler controls are:

```sh
-O2 -frename-registers -falign-jumps=16 -falign-labels=1 -falign-loops=2
```

Both historical GCC snapshots independently generated the same
**135-byte** function and verified **50 identical original-offset
bytes**, with five original relocation sites authenticated.
There is no byte-exact function acceptance from this batch.

The positive difference is a genuine source-level C++03
reconstruction of the delimiter control-flow tree:

```cpp
char c = *input;
if (c == 44) goto endword;
if (c <= 44) {
    if (c == 9) goto endword;
    if (c > 9) {
        if (c == 32) goto endword;
    } else if (c == 0) goto endword;
} else {
    if (c == 46) goto endword;
    if (c == 91) goto endword;
}
```

The original x86 output compares byte `DL` to 44 and 9 and uses a
two-level branch tree for the low/high groups. Its delimiter set is
NUL, tab, space, comma, period and left bracket; leading whitespace
also includes newline, but newline **inside** the word is *not*
a delimiter. A Linux i386 GCC `char` is signed by default, so the
nested comparisons compile into byte-sized signed compare branches
rather than arbitrary 32-bit dispatch operations. The reconstructed
source preserves those static input/output semantics. Byte identity
alone cannot prove which exact C++03 spelling the 2004 author wrote.

The original scanner uses **EAX** for the `currentPosition` local,
**ECX** for the input pointer and **EBX** for word destination. The
selected source still has the first two reversed, so the contiguous
byte-exact entry prefix is only **one byte** and the first mismatch
remains at original relative `+1`. Most of the improved 50 matching
offsets occur in the valid case-control region. Do not confuse this
original-offset score with a 50-byte matched prefix or whole-function
equivalence.

Comparing the old and new original-offset equality masks reveals
**12 common matching offsets**, **38 newly matching offsets**, and
**11 old matching offsets lost**. Thus the improvement is a net
**+27** matching bytes, not a 27-byte extension of a contiguous prefix.
Most new positions occur in the genuine case comparison and branch tree.

The five *relocation sites* target three named original globals:
`currentPosition` at `0x0808bc68`, `wordPosition` at
`0x0808bc70`, and `word` at `0x08077440`. Two additional uses of
`currentPosition` cause the number of sites to exceed the number of
unique targets. The scorer reconstructs authentic relocations; no
literal bytes need a separate mapping.

## Source-level and historical-compiler experiments

Compiled C++03 recipe matrices are reproduced through checked-in
`vuc/probes/evaluate_*_oct15f.py` drivers. Each saves the generated
source `.cpp`, historical compiler objects, and scored
`results.json` under independent guest
`/tmp/vuc-binutils215/` directories.
All positive scores below require exact **135-byte** length and all
**5** original relocation sites. Wrong-length overlap is not counted.

| Experiment | Recipes | Original-length eligible | Best verified match |
| --- | ---: | ---: | ---: |
| `evaluate_getnextword_source_lifetimes_oct15f.py` | 650 | 325 | 23/135 |
| `evaluate_getnextword_return_buffer_lifetimes_oct15f.py` | 650 | 290 | 23/135 |
| `evaluate_getnext_switch_byte_tree_oct15f.py` | 500 | 64 | **25/135** |
| `evaluate_getnext_byte_tree_alignment_oct15f.py` | 320 | 48 | **36/135** |
| Signed leading-space bounds (guest scratch `getnext-signed-leading-oct15f`) | 108 | Some original-sized variants | 36/135 |
| `evaluate_getnext_range_tree_topology_oct15f.py` | 351 | 108 | **50/135** |
| `evaluate_getnext_50_lifetimes_oct15f.py` | 480 | 318 | 50/135 |

A genuine `char` delimiter nested-tree source with period GCC
loop alignment 2 recovers more of the original branch layout
than explicit Boolean chains, `switch`-based dispatch,
independent early returns or rearrangements of output/return
pointer liveness. Crucially, the original-size 50-byte
source is **not** reproduced simply by changing a single
`-falign` option on the old 23-byte source: both source
control-flow organization and historical compiler placement matter.
The final 480-case C++03 return/position/update-lifetime matrix uses a
fixed-seed, reproducible sample across all relevant source families,
rather than truncating the lexicographically ordered matrix at one
declaration spelling. None beats the selected 50/135 match.

No invalid larger or smaller compiler body is promoted on the
strength of overlapping bytes. No C/C++ code or symbols belonging
to the parallel VCL implementation are edited.

## Independent reproduction of the accepted experimental candidate

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -falign-jumps=16 \
    -falign-labels=1 -falign-loops=2 \
    -c "$R/probes/GetNextWordNestedRangePartialOct15f.cpp" \
    -o "/tmp/vuc-binutils215/getnext-final50-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/getnext-final50-${snap}.o" \
    --config "$R/config/get_next_word_partial_oct15e_probe.json"
done
```

Expected for **each**:
`length=135`, `exact_bytes=50`, `relocations=5`,
`exact=false`; therefore it remains excluded from
`vuc/config/exact_candidates.json`.

## Other near-exact function control

A separate bounded source-shape matrix
`vuc/probes/evaluate_yycreate_error_guard_oct15f.py`
tries legitimate C++03 `yyFlexLexer::yy_create_buffer`
malloc-failure guards and error-return branch layouts. The original
function is `0x080617b0`, 139 bytes, with five authentic
relocations and 44 verified literal bytes; its selected source
already has **131/139**. The complete matrix compiled **420**
source/compiler recipes: **276** retained the original 139-byte
size, all five relocations and all 44 literal bytes. The best stayed
**131/139**; no candidate exceeded the selected partial, so the
existing yyFlexLexer source, status and config are unchanged. The
other 144 attempts were rejected by the strict gate. The executable
and reproducible full score ledger are retained in
`/tmp/vuc-binutils215/yycreate-error-guard-oct15f/` in the Linux guest.

## Acceptance and next directions

The overall original ELF named function ledger remains
**201 EXACT / 25 EXPERIMENTAL / 72 UNRESOLVED** of 298, with
**15,007 fully original-byte-matched code bytes**. No new exact
functions were recovered in this batch; this is a meaningful
**+27 matching original bytes** within one original-length partial.

Further progress on the scanner likely requires authentic parameter
and position register lifetime evidence (not manually binding
registers), or original InlineAsm.cpp translation-unit provenance.
The source-level scanner semantics should also be independently
validated against the executable before claiming behavioral
equivalence on arbitrary input. The exact matcher is the authority
for per-function instruction bytes and relocations, while whole
linked-ELF byte matching remains unavailable.

The historical acceptance gates are mandatory after any rebase:
`python3 -m unittest discover -s vuc/tests -q`,
`vuc/scripts/match_candidates.py`,
`match_data_candidates.py`, `match_bss_candidates.py`,
and `git diff --check`.

At the conclusion of this batch, all **173 host unit tests** passed;
the exact code gate returned **201/201 fully identical functions** and
**15,007/15,007 byte-exact accepted instruction bytes**, initialized
data matched **244/244** objects, and BSS matched **12/12** source
declarations. The selected partial independently reproduced
**50/135 and five genuine relocation sites** with each of the two
recovered period compilers. These are per-function acceptance metrics,
not a whole-ELF binary-identity result.

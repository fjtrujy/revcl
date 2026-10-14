# VUC 2004.1 — MLGameString fragment-list index lifetime (2026-10-14)

Branch: `vuc`. Historical target: original VUC 2004.1 i386 Linux ELF from
the unchanged tracked original ZIP. VM: `revcl-oracle`. Compiler wrappers:
`/tmp/vuc-binutils215/g++-jan25-as215` (GCC 3.3.3 prerelease,
January 25, 2004), `g++-dec06-as215` (December 6, 2003), GNU as 2.15.

## Mandatory rebasing and new VCL main lessons

`git fetch origin && git rebase origin/main` incorporated two new commits:
`1eff5c3` (VCL `optDumb2.cpp`) and `42dca3b` (web progress viewer).
The latest VCL success isolates a **genuine unchanged function in a new
compiler unit** and changes the GCC 3.2.2 peephole/rnreg scratch state;
its report expressly distinguishes a user-authorized partition from
historical original-owner evidence. That mechanism cannot be assumed for
the different GCC 3.3.3 VUC executable, and no VUC unit split was performed.
The 163 pre-existing untracked paths, original ZIP and root VCL source
were preserved throughout the work.

## One newly improved, independently authenticated partial

`TranslateFragList<MLGameString>(ASTNodeList<MLGameString>&)`
at original address **`0x0806e5b0`**, original length **306 bytes**:

| Requirement | Prior | Now |
| --- | ---: | ---: |
| Original-offset identical instruction bytes, original-length object | **72/306** | **75/306** |
| Contiguous original-offset exact entry prefix | **2 bytes** | **10 bytes** |
| Authenticated original linker/compiler relocations | 6/6 | **6/6** |
| Accepted byte-exact | No | **No** |
| Central ledger classification | EXPERIMENTAL | **EXPERIMENTAL** |

The strictly original-sized new source is
`vuc/probes/translate_frag_list_string_partial_oct14.cpp`; its existing
manifest remains `vuc/config/translate_frag_list_string_oct14_probe.json`
and the accepted exact manifest is unchanged. Flags:
`-O2 -frename-registers -falign-jumps=1`.

The original assembly has the input-list induction update at
`0x0806e6a5` **after fragment construction and list growth/copying**.
The previous source advanced `i` before constructing its IR fragment;
moving the update **after fragment construction but before the semantic
`output.Add(fragment)` call** changes its GCC 3.3.3 register/stack
lifetimes, recovering the 10-byte common prefix and three additional
matching bytes. Moving it *after the entire append operation* is a
separate, semantically equivalent experiment: it matches only a
nonoriginal-length **317-byte** output (93 overlapping bytes in that
317-byte output); it is **not** a candidate for the 306-byte contract.

All six authentic relocations remain resolved against the original ELF:
`operator new(unsigned)`, the IR Fragment_Data vtable, the MLGameString
copy constructor, `operator new[](unsigned)`, `operator delete(void*)`,
and `_Unwind_Resume`. The recovered source is still not equivalent at
most bytes or in frame size; this is **not** an exact function or
evidence of original compiler flag/branch-hint authorship.

Independent recompilation of the *tracked chosen source*, not a retained
search-matrix output, confirmed for **both GCC 3.3.3 snapshots**:
`length=306`, `exact_bytes=75`, `exact=false`, and
`relocations=6`. No manual assembly, original binary bytes, register
constraints, fake relocation or executable padding was inserted.

## Bounded additional source/compiler evidence

Three new source-controlled drivers construct only ordinary plausible C++03
source variations. Their generated source/object files and full JSON
results are in the existing VM scratch under `/tmp/vuc-binutils215`,
not in the repository.

| Driver in `vuc/probes/` | Historical compiler recipes | Result |
| --- | ---: | --- |
| `evaluate_string_fraglist_index_lifetime_next_oct14.py` | **144** | After-construction/just-before-append update: **75/306** and 10-byte prefix; after-add variant is 317 bytes. All scored original-function relocation targets checked |
| `evaluate_string_fraglist_growth_hints_next_oct14.py` | **72** | Real growth and old-storage predicates with GCC branch-frequency hypotheses and jump alignment. Original-length best remains **75/306**; only j1/plain growth and plain/likely copy achieve the best score |
| `evaluate_hashcopy_index_references_next_oct14.py` | **70** | Both GCC snapshots; source-level `int&`, `const int&`, index-pointer and copied-index owners; no original-sized **166-byte** `MLHashTable` copy-ctor candidate. Sizes 147, 150, 152, 154, 162, 170, 181. Genuine 4/4 relocation candidates exist at wrong lengths |

The new string-index experiment intentionally varies *when the local
list iteration index is updated*: before constructing, after constructing,
immediately before append, and after append. It does not assert the
selected late-increment position is proven original author source, only
that its valid source-level lifetime shifts improve original ELF bytes.

The copy constructor at `0x0804f420` still lacks its historical stack
slot/index lifetime. Indirecting a nonvolatile `int` through an ordinary
reference generally folds back to the prior register-induction output,
whereas copied-index forms change emitted length/relocations. Neither
clue warrants declaring the constructor byte-exact.

## Reproduce exact partial and regression

```sh
R=/path/to/revcl/vuc

for snapshot in jan25 dec06; do
  limactl shell revcl-oracle -- "/tmp/vuc-binutils215/g++-$snapshot-as215" \
    -O2 -frename-registers -falign-jumps=1 \
    -c "$R/probes/translate_frag_list_string_partial_oct14.cpp" \
    -o "/tmp/vuc-binutils215/fragstring-75-$snapshot.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/fragstring-75-$snapshot.o" \
    --config "$R/config/translate_frag_list_string_oct14_probe.json"
done

limactl shell revcl-oracle -- python3 "$R/probes/evaluate_string_fraglist_index_lifetime_next_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_string_fraglist_growth_hints_next_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_hashcopy_index_references_next_oct14.py"

python3 -m unittest discover -s vuc/tests -q
```

The complete baseline/retained acceptance gates are
`199/199` exact functions with `14,621/14,621` authenticated
instruction bytes, `244/244` initialized-data objects with
`71,536/71,536` original bytes, and `12/12` named BSS declarations.
The inventory stays **199 EXACT / 22 EXPERIMENTAL / 77 UNRESOLVED**
out of 298; this update changes only the quality of one *experimental*
candidate. Whole-ELF binary identity remains unestablished.

## Next investigations

Recover the original three-field `ASTNodeList::Add()` source/inline
definition and the **i stack slot/ECX vs EDX** first-block lifetime:
the selected candidate still uses a larger function stack frame
than the original. Do not repeat generic branch-hint matrices without
a new genuine source owner or branch fact. For the 166-byte hash-table
constructor, focus on original node-tail references and EH lifetimes
rather than nonthrowing string-copy annotations, which previously
removed genuine cleanup calls.

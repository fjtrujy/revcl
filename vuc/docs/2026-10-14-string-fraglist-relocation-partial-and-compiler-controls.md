# VUC: 72/306 authenticated string fragment-list partial and GCC 3.3 controls

Date: 2026-10-14. Branch: `vuc`. Original target: verified VUC 2004.1
Linux i386 executable extracted in memory from the canonical tracked ZIP.
The historical build controls use both recovered GCC 3.3.3 prereleases
(2004-01-25 and 2003-12-06) with GNU assembler 2.15.

## Repository and baseline

Fetched `origin` and rebased the existing `vuc` checkout onto
`origin/main` at `62fdece` without conflicts. The prior 163 untracked
paths remained untouched and no worktree was created. The existing 126 VUC
commits replayed; the last previous VUC scope-copy commit is now `d2b7071`.
The strict post-rebase baseline remained **198/198 EXACT functions**,
**14,501/14,501 verified instruction bytes**, **244/244 named initialized
data objects (71,536 bytes)**, **12/12 BSS declarations**, and **156**
host tests.

The newly incorporated VCL commits explain three distinct GCC 3.2.2 phenomena:
compiler-generated branch-alignment from a real loop continuation hint
(`6a42bca`), dead-register scratch changes at a real source-unit boundary
(`f471a02`), and altered live operand lifetimes with
`-fno-cse-follow-jumps` (`62fdece`). These are evidence-based hypotheses
to test, not automatically valid GCC 3.3.3 fixes or recovered VUC flags.

## Newly authenticated original-length partial

| Property | Independently measured |
| --- | --- |
| Original named symbol | `TranslateFragList<MLGameString>(ASTNodeList<MLGameString>&)` |
| Original address | `0x0806e5b0` |
| Original instruction length | **306 bytes** |
| Previously tracked status | **UNRESOLVED**, no original-length selected candidate |
| New independent match | **72/306 original-offset instruction bytes** |
| Candidate function length | **306 bytes** |
| Original external relocations | **6/6 authenticated** |
| Candidate status | **EXPERIMENTAL, not exact** |

Source: `vuc/probes/translate_frag_list_string_partial_oct14.cpp`.
Manifest: `vuc/config/translate_frag_list_string_oct14_probe.json`.
Flags: `-O2 -frename-registers -falign-jumps=1`.

All six real relocation targets are independently checked: `operator new`
at `0x080499ec`, `IR::Fragment_Data` vtable at `0x08076068`,
`MLGameString` copy constructor at `0x08071770`, `operator new[]`
at `0x080499ac`, `operator delete` at `0x0804973c`, and
`_Unwind_Resume` at `0x0804998c`. No relocation address was invented
or inferred from a candidate's byte count.

The ordinary C++03 source allocates the 12-byte IR fragment, constructs
its string member from the correct input, and appends the result to a
three-field result list (length, capacity, pointer array). It reproduces
16-element initial capacity, capacity doubling, exception-construction
cleanup and compiler-generated `cld; rep movsl` array copying. The selected
variant predicts an outer loop's genuine `i < input.length` condition using
`__builtin_expect(..., 1)`; this compiler option is a *hypothesis about
branch layout*, not evidence the original author wrote the hint.

The current match remains structurally incomplete: the original starts
with `xor ecx,ecx` while the candidate uses `xor edx,edx`; the original
reserves 0x1c bytes of stack while the candidate reserves 0x2c. Allocation,
initialization, the loop entrance and the exceptional cleanup differ.
The common `rep movsl` idiom alone does not establish true object ownership
or source equivalence. The candidate must **not** move to `vuc/src/` or
`vuc/config/exact_candidates.json`.

A saved independent compilation of the selected source with each of the
2004-01-25 and 2003-12-06 GCC 3.3.3 wrappers returned precisely
`length=306`, `exact_bytes=72`, `relocations=6`, `exact=false`.
The target-size result is not inferred from the search matrix alone.

## Negative controls and bounded follow-up work

| Reproducible driver in `vuc/probes/` | Historical compiler recipes | Outcome |
| --- | ---: | --- |
| `evaluate_fraglist_string_lifetimes_oct14.py` | **504** | Several correct-length candidates; best original-length score only 24/306 with scalar copy; real `memcpy` gives 89 overlapping bytes but length 315 |
| `evaluate_fraglist_string_source_unit_oct14.py` | **64** | Recompiles real `TranslateFrag.cpp` predecessor plus target; no 306-byte output, best nonoriginal-length overlap 89/315 |
| `evaluate_fraglist_string_branch_likelihood_oct14.py` | **216** | Three genuine predicate predictions, jump alignment, block-reorder; recovered the selected **72/306** original-length candidate with all six relocations |
| `evaluate_scope_copy_compiler_passes_oct14.py` | **60** | January/December GCC 3.3.3, ten real optimizer-pass hypotheses and three jump alignments; no original 120-byte scope-copy candidate |

Total **844** compiler recipes in the retained complete driver matrices,
excluding independent final recompilations. All generated C++ sources,
ELF objects, and JSON search results are held inside the existing VM under
`/tmp/vuc-binutils215/{fraglist-string-lifetime-oct14,fraglist-string-source-owner-oct14,fraglist-string-branch-hints-oct14,scope-compiler-passes-oct14}/`.
No test alters the ELF, archives or VCL project. All long-term recipe
generators are saved as source-controlled Python files.

The unrelated `std::__uninitialized_copy_aux<Scope*,Scope*>` at
`0x0804f220` remains a **119-byte** compiler output for a **120-byte**
original: **75 exact prefix bytes**, 84 original-offset overlaps and six
original relocations. No tested optimization control changes that residual.
The prior scope-copy diagnostic is still excluded from both selected and
exact manifests.

## Reproduction

From the Mac checkout, with the existing `revcl-oracle` guest:

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
  -falign-jumps=1 -c "$R/probes/translate_frag_list_string_partial_oct14.cpp" \
  -o /tmp/vuc-binutils215/fraglist-string-independent-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/fraglist-string-independent-oct14.o \
  --config "$R/config/translate_frag_list_string_oct14_probe.json"
```

Substitute `g++-dec06-as215` to independently check the older GCC 3.3.3
snapshot; the expected result is the same 72/306, with 6 relocations.
For all bounded negative controls, invoke each corresponding evaluator
under `limactl shell revcl-oracle -- python3 "$R/probes/<driver>.py"`.

Run strict accepted-function, named-initialized-data, BSS, host unit tests,
`coverage.py --write` and `git diff --check` to verify no accepted match
regressed. The new ledger is **198 EXACT / 22 EXPERIMENTAL / 78
UNRESOLVED** of 298 named functions. The **14,501** independently accepted
exact bytes remain unchanged. Full linked VUC executable byte equivalence
is **not established**.

## Next investigation

Compare the original and candidate GCC 3.3 early-RTL variable lifetimes
before exploring more predictions. The genuine `Translate.cpp` source-unit
ownership and `ASTNodeList::Add` implementation have not been recovered;
a first difference at the function's second instruction cannot be resolved
by moving alignment padding. For the Scope copy exception path, recover
the exact `catch` cleanup iterator semantics and `.gcc_except_table`
landing ranges, rather than repeating the exhausted compiler-pass matrix.

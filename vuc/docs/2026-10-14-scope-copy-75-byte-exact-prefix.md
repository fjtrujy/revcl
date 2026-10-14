# VUC scope-copy prefix and constrained STL source controls

Date: 2026-10-14. Repository: `/path/to/revcl`,
branch `vuc`; baseline `66d07ef`.
Authority: unchanged original VUC 2004.1 Linux ELF32 i386 archived binary.
Compiler: historical GCC 3.3.3 (2004-01-25 prerelease), GNU as 2.15
inside the existing `revcl-oracle` Lima guest.
An independent compilation using the recovered 2003-12-06 GCC 3.3.3
prerelease also yields precisely 119 emitted bytes, 75 exact prefix
bytes, 84 matching original-offset bytes and six resolved relocations.

## New independently verified byte-exact partial prefix

The *unresolved* original function

`std::__uninitialized_copy_aux<Scope*, Scope*>()`
at **`0x0804f220` (120 original bytes)**,

where `Scope` is `MLHashTable<MLGameString, SymbolTableEntry*,
MLHashFun<MLGameString>, 513>`, now has a saved and independently
recompiled **119-byte C++03 diagnostic source**:

`vuc/probes/scope_copy_119byte_partial_oct14.cpp`.

| Measurement | Independently reproduced |
| --- | ---: |
| Original function size | 120 |
| Historical compiler emitted size | **119** |
| **Contiguous exact original-offset prefix** | **75 bytes** |
| Matching original-offset bytes across overlapping regions | **84** |
| Authenticated external relocation sites | **6/6** |
| Complete byte-exact function | **NO** |
| Accepted/selected reconstruction manifest | **NOT listed** |

The recovered prefix covers the original function prologue and complete
forward scope-copy iteration. It calls the real
`MLHashTable::MLHashTable(const MLHashTable&)` at `0x0804f420`,
advances both 0x804-byte scope pointers, and reaches the original
`__cxa_begin_catch` call. The six original relocation targets
also include `MLHashTable::clear()` at `0x0804f020`,
`__cxa_rethrow`, `__cxa_end_catch`, and `_Unwind_Resume`.

**Strict acceptance remains deliberately blocked.** The generated
119-byte body is one byte short of the original 120-byte body, with
mismatches from original offset 75 onward. An exact common prefix is
a useful partial *function reconstruction*, but cannot be treated as
a new exact function or even an original-length experimental candidate.
The candidate remains `UNRESOLVED` in the central ledger.
The existing `match_probe.py` correctly rejects it with
`function lengths differ: 119 vs 120`. No scorer, relocation
or archive bytes were modified.

The persistent diagnostic configuration
`vuc/config/scope_copy_prefix_diagnostic_oct14_probe.json` describes
the original 120-byte contract, six authentic relocations, and
`-O2 -frename-registers -falign-jumps=8`. It is intentionally
**excluded from both** `selected_candidates.json` and
`exact_candidates.json`. A host unit test enforces that exclusion.

The independent diagnostic
`vuc/probes/evaluate_scope_copy_prefix_oct14.py` compiles the saved
source, normalizes its six relocations against original ELF addresses,
then checks the **75-byte prefix**, the 84 overlapping-offset matches,
and the two lengths. This is a forensic comparator only; it does
not bypass or change the strict 120-byte acceptance test.

## Four bounded, real compiler search spaces

Four further source-level investigations were fully executed, using the
historical GCC/assembler and unchanged original ELF as authority.
Scratch objects/sources remain under `/tmp/vuc-binutils215/` inside
the guest, not committed as generated artifacts.

| Driver (under `vuc/probes/`) | Recipes | Results |
| --- | ---: | --- |
| `evaluate_scope_copy_cleanup_lifetimes_oct14.py` | **360** | 58 candidates were 119 bytes, none 120; best forensic 84 matching overlapping bytes with 75 exact initial bytes |
| `evaluate_hash_copy_branch_source_oct14.py` | **336** | No original-length 166-byte `MLHashTable` copy constructor; generated lengths include 162 and 163 |
| `evaluate_scope_iterator_lifetimes_oct14.py` | **280** | No original-length 149-byte normal-iterator specialization; output lengths include 119, 112, 100 and 92 |
| `evaluate_fraglist_memcpy_guards_oct14.py` | **144** | No original-length 240-byte candidate; genuine `__builtin_memcpy` bodies were predominantly 249/252/233 bytes |

**Total: 1,120 compiler configurations**, zero accepted exact
functions and zero original-length improvements.

The cleanup experiment varied *real C++03 exception-handling code*:
forward `for` loops, `while`, saved iterators, increment order,
typed iterator versus pointer and `__builtin_expect` probability,
alongside genuine GCC alignment, branch and reorder controls.
It did **not** insert inline assembly, fake symbols or executable
padding to manufacture the original 5-byte cleanup-loop alignment
gap. GCC's exception lowering still differs.

The constructor experiment changed the genuine 513-bucket loop
and linked-node branch expectations. The iterator-copy experiment
used an explicit, ordinary C++03 specialization with placement new
and typed iterator cleanup; its generated functions were shorter than
the original 149-byte iterator body. The list-copy experiment
preserved real semantics while checking optional zero-size guards
around GCC's `rep movsl` emission; length remained wrong.

**A rejected hypothesis is not an exact match.** The only newly
authenticated contiguous prefix is the 75-byte scope-copy prefix
above; the other source families yielded no improved original-length
candidate. Do not recycle these combinations without new
source-unit, EH or compiler-state evidence.

## Reproduction and constraints

On the Mac, with the connected `revcl-oracle` guest:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_scope_copy_prefix_oct14.py"
```

Expected: `original_length=120`, `candidate_length=119`,
`exact_common_prefix=75`,
`identical_overlapping_offsets=84`, `relocations_verified=6`,
`scored_as_exact=false`.

The accepted VUC inventory is unchanged at **198 EXACT /
21 EXPERIMENTAL / 79 UNRESOLVED** (out of 298 named functions).
All 198 historically accepted exact-function matches, 244
initialized-data objects, and 12 BSS declarations must remain green
before committing. No whole-linked-ELF equality is established.
Pre-existing untracked user files, the upstream binaries, and the
root VCL project must remain untouched.

## Next investigation

The remaining scope-copy byte difference is concentrated in the
`catch` cleanup loop's instruction alignment and when the iterator
is advanced relative to `MLHashTable::clear()`. Recover the
actual original template-instantiation source unit, its earlier
symbols and exception metadata. A five-byte missing region is
**not** permission to add code padding.

Alternatively, pursue a new, unrelated original function whose
constructor/lifetime structure is better known rather than retesting
these exact source-expression and alignment combinations.

# VUC: scope-iterator EH layout and copy-constructor exception control (2026-10-14)

Branch: `vuc`. Rebased after `git fetch origin`; `origin/main` is
unchanged from `b95fc41`, so `git rebase origin/main` was a no-op.
The working tree initially had **163 pre-existing untracked paths**, all
preserved. No original ZIP/ELF, root VCL source, or accepted VUC source was
modified. Compiler: both historical GCC 3.3.3 prereleases (Jan 25, 2004 and
Dec 6, 2003), with the recovered GNU as 2.15 in `revcl-oracle`.

## Outcome and strictly measured evidence

**No new function satisfied the original-size and original-relocation
acceptance contract.** The accepted inventory stays **199 EXACT /
22 EXPERIMENTAL / 77 UNRESOLVED** of 298 original named functions,
and **14,621/14,621** verified accepted instruction bytes.

The main actionable finding is that the STL `std::__uninitialized_copy_aux`
instantiation with genuine `__gnu_cxx::__normal_iterator<Scope>`
has the correct instruction/data-callee graph (six genuine relocations)
but differs from its original 149-byte implementation predominantly at
compiler-selected alignment and branch displacements.

### 1. Normal-iterator scope-copy target: genuine template lifetime

- Original: `0x0806a840`, length **149**.
- Header: historical `bits/stl_uninitialized.h`, as supplied by the
  restored GCC 3.3 toolchain. Uses `_Construct(&*__cur, *__first)`,
  `_Destroy(__result,__cur)` and exception rethrow; these details matter
  to the EH graph.
- Source: `vuc/probes/scope_iterator_clear_throw_oct14.cpp`.
  It uses the same `clear() throw()` declaration in
  `vuc/src/ScopeCopyTable.h` that independently restored the exact
  120-byte pointer specialization.
- Correctly typed, implicit historical libstdc++ instantiation:
  **136 emitted bytes**, six resolved original relocations, **28-byte exact
  prefix**, **73 identical bytes at original offsets in the 136-byte
  overlap**. This is NOT an original-sized accepted or registered partial.
- Genuine gap in original: **9 bytes** of compiler-generated alignment
  at relative **+0x47 through +0x4f**, before the return epilogue at
  **+0x50**; additionally **4 bytes** at **+0x5c through +0x5f**,
  preceding the EH catch path at **+0x60**. The genuine 136-byte
  candidate's analogous return epilogue and catch path occur at
  **+0x47** and **+0x53**. These 13 bytes explain the *size* delta;
  they do not make the instruction-level branch displacements equal.
  We must not add hand-authored padding or patch original opcode bytes.

`vuc/probes/evaluate_scope_iterator_throw_alignments_oct14.py`
ran **256** natural GCC recipes: both historical snapshots, four
jump-align values (1/4/8/16), four label-align values (1/2/4/8),
four loop-align values (1/2/4/8), and reordered vs original-order
basic blocks. All six relocations were independently processed using
the original ELF comparator. Emitted sizes included **136, 139, 140,
141, 142, 144-148, 152, 156, 159, 162, 164, 168, 172**; **none
was the required 149 bytes**. The best forensic comparison stays
73/136 with the 28-byte contiguous prefix. Scratch details:
`/tmp/vuc-binutils215/scope-iterator-alignment-throw-oct14/results.json`
inside the Lima VM.

### 2. Source-faithful _Construct/_Destroy branch hypotheses

`vuc/probes/evaluate_scope_iterator_stl_construct_oct14.py` additionally
tests **112** C++03 source recipes in an explicit specialization using
the *real historical* `_Construct`, `_Destroy`,
`__throw_exception_again` rather than approximating their behavior
with manual placement-new and loops. Predicates include ordinary
`first!=last`, true/false `__builtin_expect`, and negated
first/last comparison, in genuine `for`, `while`, postincrement
and guarded `do` forms, under seven compiler options.

All compiled well-formed candidates range **136-177 bytes** (selected
136-byte source matches 73 overlapping original-offset bytes with
6/6 authentic relocations). Again **no original-length 149-byte
candidate**. The C++ branch hints are *compiler hypotheses*, not
evidence that the historical author wrote them. Generated source and
objects are located in
`/tmp/vuc-binutils215/scope-iterator-real-stl-helpers-oct14`.

### 3. Real MLGameString copy constructor and EH exception declaration

The original `MLGameString(const MLGameString&)` at `0x08071770`
has an 15-byte body that simply copies its pointer field (no calls).
It is therefore natural to test, *but not assume*, a historical C++03
no-throw declaration, even though the current provisional source
declaration does not specify one.

`vuc/probes/evaluate_hashcopy_nothrow_declarations_oct14.py`
tests **36** compile recipes: a plain, `throw()`, or GCC
`__attribute__((nothrow))` declaration for this genuine copy
constructor; optional no-throw declaration of its parent 513-bucket
table constructor; six basic GCC alignment/control modes. All changes
are made to scratch-header copies, never the accepted shared headers.
The target table copy constructor at `0x0804f420` has original size
**166 bytes** and **four** original external relocations.

The `MLGameString` copy-constructor `throw()` and `nothrow`
variants emit a **125-byte** hash copy constructor with **only
2/4** original call relocations; critically, they lose the original
cleanup calls `operator delete` and `_Unwind_Resume`. Ordinary
declarations retain four genuine relocations in some
**162-byte** outputs but still miss the original 166-byte target.
No proposed header annotation is therefore promoted. Even though
the callee body cannot throw, the original caller's observed EH
code is evidence that this non-throwing property was not necessarily
visible through its declared exception specification.

Reproducible generated data live in
`/tmp/vuc-binutils215/hashcopy-real-nothrow-oct14/results.json`.

## Why these results matter

Two closely related original STL routines do **not** share the same
matching outcome. The 120-byte pointer specialization is fully exact
using a period-plausible `clear() throw()` declaration; the
149-byte normal-iterator specialization still lacks its original
compiler-selected label locations. The original's 13-byte padding
is *inside* the symbol, not linker padding between functions.

The best next experiment is a legitimate change to **compiler
basic-block probabilities or original translation-unit processing
context** justified by real source/ELF evidence, followed by
branch-label comparisons at the two concrete boundaries above. Avoid
another undirected alignment-only matrix; those combinations are
already checked. The 166-byte hash constructor instead requires
the original stack-owned bucket index and pointer-tail lifetimes;
declaring the MLGameString constructor nonthrowing removes authentic EH.

## Reproduce and acceptance gates

Run from the user's existing Mac terminal against the bound project:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_hashcopy_nothrow_declarations_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_scope_iterator_throw_alignments_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_scope_iterator_stl_construct_oct14.py"
python3 -m unittest discover -s vuc/tests -q
limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" \
    --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
    --out /tmp/vuc-binutils215/next-batch-exact-oct14
limactl shell revcl-oracle -- python3 "$R/scripts/match_data_candidates.py" \
    --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
    --out /tmp/vuc-binutils215/next-batch-data-oct14
limactl shell revcl-oracle -- python3 "$R/scripts/match_bss_candidates.py" \
    --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
    --out /tmp/vuc-binutils215/next-batch-bss-oct14
```

These batch-level controls do not establish whole-linked-executable
binary equivalence.

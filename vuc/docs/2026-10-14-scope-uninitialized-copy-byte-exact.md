# VUC: byte-exact std::uninitialized_copy of 513-bucket symbol scopes

Date: 2026-10-14. Branch: `vuc`. Original authority: the unchanged
ZIP-backed VUC 2004.1 Linux x86 ELF, at `0x0804f220`. Compiler authority:
recovered GCC 3.3.3 prereleases 2004-01-25 and 2003-12-06 with GNU as 2.15.

## Verified new exact function

| Measurement | Before | After |
| --- | ---: | ---: |
| `std::__uninitialized_copy_aux<Scope*, Scope*>` | 75-byte contiguous original-offset prefix; 84 matching overlapping bytes in a **119-byte** candidate | **120/120 byte-exact** |
| Original instruction size | 120 | **120** |
| All original call relocations authenticated | 6 | **6/6** |
| Accepted status | UNRESOLVED (diagnostic source only) | **EXACT** |

The accepted `vuc/src/ScopeUninitializedCopy.cpp` reconstructs a normal
C++03 pointer-input loop, placement construction of each 513-bucket scope
(`sizeof(Scope) == 0x804`), and cleanup of successfully constructed
objects if copying throws. Its real external calls resolve to the original
addresses:

| Symbol | Original VA |
| --- | --- |
| `MLHashTable<...>::MLHashTable(const&)` | `0x0804f420` |
| `MLHashTable<...>::clear()` | `0x0804f020` |
| `__cxa_begin_catch` | `0x0804995c` |
| `__cxa_rethrow` | `0x0804979c` |
| `__cxa_end_catch` | `0x0804983c` |
| `_Unwind_Resume` | `0x0804998c` |

A period-supported **`void clear() throw();`** declaration in the
unit-local `ScopeCopyTable.h` changes GCC 3.3.3's exception-cleanup
allocation/alignment enough to recover the missing byte and the cleanup
iteration bytes without changing the source-level loop body. Both the
2004-01-25 and 2003-12-06 snapshots independently emit exactly
`120/120`, original length and all six relocations, with
`-O2 -frename-registers -falign-jumps=4`. The previous ordinary declaration
emitted 119 bytes with the same genuine call targets. No assembly, fabricated
relocation, padding source, original machine-byte patch, or binary mutation
was used.

The original VUC header's exact declaration is **not authenticated**.
The successful `throw()` spelling is plausible in 2004 C++, and the
bounded controls show that a GCC `nothrow` method attribute also yields
the matching code. The evidence therefore supports a no-throw callee
declaration at this use site, not recovery of the precise original spelling.
We preserve this original-shaped, source-unit-specific declaration separately
instead of modifying the shared provisional `MLHashTable.h`, which other
accepted functions include. A full original source-unit map remains unknown;
matching one function does not establish a whole linked ELF.

## New VCL main lesson and bounded controls

Rebased the VUC branch onto fresh `origin/main` at `b95fc41`.
The only new VCL note establishes that an actual C++ `throw()` declaration
can alter useful GCC 3.2 exception and register codegen, while simply
accepting a `nothrow` attribute does not prove it affects that particular
function. This is not itself proof of VUC GCC 3.3.3 behavior; the independently
compiled target ELF comparisons above establish its transfer.

Three independently reproducible bounded historical investigations:

- `vuc/probes/evaluate_scope_exception_specs_oct14.py`: **108** source/
  flag recipes, varying C++03 method exception specifications for the
  authentic MLHashTable constructor, clear and destructor in a scratch
  original-shape header. Correct 120-byte candidates preserve all six
  relocations, and the `clear() throw()` candidate verifies 120/120.
  The source and generated objects live in the Lima guest scratch under
  `/tmp/vuc-binutils215/scope-exception-declarations-oct14`.
- `vuc/probes/evaluate_string_fraglist_loop_structure_oct14.py`:
  **192** source/flag recipes for the 306-byte string fragment-list
  function (natural `while`, `for`, `do`, early-return and `goto`
  loop entries with GCC jump alignment). None surpasses the existing
  **72/306** original-length, 6-relocation selected experimental candidate.
  All generated objects/results are outside the repository under
  `/tmp/vuc-binutils215/string-fraglist-loop-structure-oct14`.
- `vuc/probes/evaluate_scope_iterator_throw_oct14.py`: **84** natural
  iterator-construction and cleanup shapes with the same historically supported
  `clear() throw()` declaration. None reproduces the original 149-byte
  `__uninitialized_copy_aux<normal_iterator<Scope>, ...>` at `0x0806a840`;
  explicit forms emit 86-96 bytes with only one external relocation, rather
  than the expected six. The four genuine implicit-template instantiations
  retain **6/6 relocations** but emit 136 or 142 bytes. The best diagnostic
  has a 28-byte contiguous original-offset prefix and 73 identical overlapping
  bytes in a **136-byte** body: it fails the strict 149-byte contract.
  These variants do not recover the original iterator/exception structure.
  Saved under `/tmp/vuc-binutils215/scope-iterator-throw-oct14`.

No variant of the second sweep is promoted. The canonical partial in
`vuc/probes/translate_frag_list_string_partial_oct14.cpp` remains
unchanged and outside the exact manifest.

## Independent reproduction

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- "$C" \
  -O2 -frename-registers -falign-jumps=4 \
  -c "$R/src/ScopeUninitializedCopy.cpp" \
  -o /tmp/vuc-binutils215/scope-copy-exact-independent-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/scope-copy-exact-independent-oct14.o \
  --config "$R/config/scope_copy_exact_oct14_probe.json"
```

Expected: `length:120`, `exact_bytes:120`, `exact:true`,
`different_offsets:[]`, `relocations:6`. Repeat with
`/tmp/vuc-binutils215/g++-dec06-as215` for independent verification
with the December GCC 3.3.3 snapshot. Accepted code lives in
`vuc/src/`; the exact manifest includes only the independently
original-ELF-authenticated candidate.

The accepted ledger becomes **199 EXACT / 22 EXPERIMENTAL /
77 UNRESOLVED** from 298 named functions, with **14,621**
independently exact instruction bytes (previously 14,501). Named-data and
BSS gates continue to be separately checked; no whole-program binary
comparison is available.

## Next leads

The natural STL `normal_iterator<Scope>` copy at `0x0806a840` is
149 original bytes, but the tested source does not instantiate the correct
construction/exception shape: recovering the actual 2004-era template
and iterator ownership is necessary before retesting more flags. For longer list
specializations, recover the genuine list member implementation and
loop-register lifetimes before repeating alignment sweeps.

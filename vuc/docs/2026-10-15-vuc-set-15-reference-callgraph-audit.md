# VUC 2004.1 — Original constant-value parser reference-count and code-generation audit

**Date:** 2026-10-15. **Working branch:** `vuc`.
Original authority: unchanged archived VUC 2004.1 Linux ELF32 i386.
Historical toolchains: GCC 3.3.3 2004-01-25 and 2003-12-06 prereleases
with GNU as 2.15, in the existing Lima `revcl-oracle` VM.

## Preservation and acceptance boundaries

Starting branch `vuc` at `e52fcbc`, clean tracked index and
**163 unrelated pre-existing untracked files**. Fetched and rebased
`origin/main` first; no new main commits were present then.
No `reset`, `clean`, new worktree, synthetic assembly/register binding,
machine-code blob, padding shim, modified archived ELF, or edit of
accepted `vuc/src/` functions.

`Value::Set(ASTConstValExpr const*)` at `0x0806e7d0` has **455**
original instruction bytes. Its selected experimental C++03 source
`vuc/probes/ValueConstValSetPartialOct15b.cpp` remains original-sized,
**67/455 matching original byte offsets**, compiler-emitted **13**
authenticated relocations and **11** original Boolean literal bytes.
It is **not** accepted as byte-exact source.

### Important correction: the original itself has 15 reference sites

The old experimental matcher correctly resolved all **13 references
actually emitted by its candidate object**, but did **not** establish
that the original had only 13 reference sites. The original 455-byte
binary has **13 distinct CALL instructions** and **two literal-address
immediates**, independently established by reading original i386
CALL rel32 displacements and immediate byte values directly from the
unchanged ELF. These are not speculative compiler-generated relocations:

| Original destination | Reference-site count | Description |
| --- | ---: | --- |
| `0x08071800` | 6 | `MLGameString::c_str() const` |
| `0x0804984c` | 2 | `__strtol_internal` |
| `0x080495fc` | 3 | `__strtod_internal` |
| `0x080498dc` | 1 | `strrchr` |
| `0x080498ac` | 1 | `memcpy` |
| `0x08075f78` | 1 | `"true\\0"` literal address |
| `0x0807668a` | 1 | `"false\\0"` literal address |

The currently selected experimental source compiles one shared
integer-conversion tail by optimizer control-flow merging. That
eliminates the Boolean fallback's **own** `c_str` CALL and
`__strtol_internal` CALL. Hence its 11 call sites + 2 literals = 13
authenticated candidate relocations versus 15 original references.
These missing sites contribute to its remaining 388 different bytes.
The original-location-aware strict partial scorer reports the
candidate's emitted relocation count, so do not label that count
"13/13 original sites" without this distinction.

**New independent original-ELF regression**:
`vuc/tests/test_value_constval_reference_sites.py` verifies
each of the 13 actual i386 CALL destination offsets, their full
original target addresses, both literal pointer immediates and both
original literal byte strings. This test uses raw ELF bytes, not the
reconstruction's object and not decompiler register guesses.

## Recover the original 0x88 stack frame and 15 distinct references

The archived function's prologue has:
`sub $0x88, %esp` and saves ESI/EBX/EDI at known frame slots.
Original scratch buffer starts at `-0x68(%ebp)`; the counter lives at
`-0x6c(%ebp)`. Ordinary C++03 capacity **56 or 64** can reproduce
the exact 0x88 frame. Buffer capacity is still a hypothesis: frame
location does not uniquely prove the declared source array length.

| Compilation hypothesis | Candidate length | Identical original byte offsets | Complete entry prefix | Original destinations |
| --- | ---: | ---: | ---: | ---: |
| Existing selected parser | **455/455** | **67** | 5 | 13/15 |
| 64-byte buffer, ordinary optimizer | 422 or 423 | up to 153 overlapping | **55** | 13/15 |
| 64-byte buffer, `-fno-crossjumping` | 462 or 463 | up to 175 overlapping | **55** | **15/15** |
| 64-byte buffer, Boolean second fetched in branch | 458 or 459 | up to 182 overlapping | **55** | **15/15** |
| Boolean second fetched in branch; next vector pointer materialized before memcpy | **459** | **213 overlapping** | **55** | **15/15** |

**Critical:** 213/459 is **NOT** an accepted 213/455 partial.
Four extra bytes mean the candidate has wrong function length.
It is source/ABI reconstruction evidence, not a strict original-size
score and it is deliberately excluded from
`vuc/config/selected_candidates.json` and
`vuc/config/exact_candidates.json`.

A permanent standalone diagnostic source preserving these 15 legitimate
references and correct 55-byte prologue is:
`vuc/probes/ValueConstValSet15ReferencesDiagnosticOct15c.cpp`.

The full `15/15` destination count, **459-byte** emitted length, **213
original-offset overlapping bytes**, and first **55 exact entry bytes**
were independently reproduced under **both recovered historical GCC
snapshots**, with genuine `-O2 -frename-registers -falign-jumps=4
-fno-crossjumping` and without invented instruction padding.

The original vector code updates the input pointer in ESI with an
ordinary LEA, then retains it across libc calls. The closest
reconstruction sometimes retains its updated pointer in a stack
temporary and reloads it after `__strtod_internal`; this affects
register lifetimes and branch offsets. The next proper investigation
is source-level pointer ownership / original translation-unit
compilation state. Do not solve four missing bytes by arbitrary
byte patching, assembler, synthetic register pinning or NOPs.

## Reproducible new experiments

Every driver compiles real C++03 with period GCC, uses the exact
archived ELF, resolves original named-call destinations and both
string-literal bytes where applicable, and records generated objects
and JSON score evidence in independent existing guest `/tmp`
subdirectories. Summaries explicitly reject function-length
mismatches even when overlapping bytes increase.

| Driver under `vuc/probes/` | Probed variants | Outcome |
| --- | ---: | --- |
| `evaluate_value_set_scratch_capacity_oct15c.py` | 324 | 0x88 frame and 55-entry prefix; 422-byte candidate overlaps 153 |
| `evaluate_value_set_buffer64_caseorder_oct15c.py` | 192 | No valid 455-byte improvement |
| `evaluate_value_set_buffer64_crossjump_oct15c.py` | 1,080 | Separate Boolean conversion path; up to 175 overlap in 463-byte output |
| `evaluate_value_set_boolean_literal_forms_oct15c.py` | 480 | No original-sized improvement |
| `evaluate_value_set_no_crossjump_alignment_oct15c.py` | 630 | No 455-byte candidate with all 15 references |
| `evaluate_value_set64_no_crossjump_lifetimes_oct15c.py` | 352 | No original-sized candidate |
| `evaluate_value_set_buffer64_caseorder_nocross_oct15c.py` | 192 | No original-sized candidate |
| `evaluate_value_set_boolean_nocross_oct15c.py` | 560 | 15-reference 458/459-byte cases; no valid 455 |
| `evaluate_value_set_type_likelihood_oct15c.py` | 35 | No 455-byte 15-reference candidate |
| `evaluate_value_set_nocross_vector_lifetimes_oct15c.py` | 170 | **213 original-offset overlap in 459-byte function, 15 references** |
| `evaluate_value_set_213_compiler_controls_oct15c.py` | 96 | No original-size improvements |
| `evaluate_value_set_459_pointer_lifetimes_oct15c.py` | 168 | Natural C++03 register hints / pointer owner controls; 459 bytes remain |
| `evaluate_value_tostring_buffer_length_oct15c.py` | 264 | Existing `Value::ToString` **159/221** unchanged, 8 relocations, 82 literal bytes |
| `evaluate_stmt_list_crossjump_probabilities_oct15c.py` | 96 | Existing `Translate(ASTStmtList)` **60/119** unchanged |

These **4,639** recipes include wrong-sized or rejected candidates,
so the count is not a number of newly matched functions.

An additional cross-family compiler-pass transfer sweep,
`vuc/probes/evaluate_experimental_gcc_pass_transfer_oct15c.py`,
tested **325 recipes across 25 selected experimental sources**. It found
**no strict original-length gains**. Together the documented matrices
investigate **4,964 compiler/source recipes**, without treating invalid
lengths, unresolved literals or compilation failures as successful matches.

## Reproduce raw original reference audit

```sh
python3 -m unittest discover \
    -s vuc/tests -p test_value_constval_reference_sites.py -v

limactl shell revcl-oracle -- python3 \
    /path/to/revcl/vuc/probes/evaluate_value_set_15site_diagnostic_oct15c.py
```

Reproduce the 459-byte diagnostic in the **existing** guest:

```sh
R=/path/to/revcl/vuc
for snapshot in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snapshot}-as215 \
    -O2 -frename-registers -falign-jumps=4 -fno-crossjumping \
    -c "$R/probes/ValueConstValSet15ReferencesDiagnosticOct15c.cpp" \
    -o "/tmp/vuc-binutils215/setdiag15-${snapshot}.o"
done
```

The strict **original-length** accepted manifest is unchanged at
200 exact functions / 14,930 byte-exact instruction bytes.
The diagnostic inventory remains 200 EXACT / 25 EXPERIMENTAL /
73 UNRESOLVED of the original 298 named functions.
No whole-linked-ELF identity is claimed.

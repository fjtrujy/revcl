# VUC — First original-sized partial for Value::Set(ASTConstValExpr const*)

**Date:** 2026-10-15. **Branch:** `vuc`. Original authority:
unmodified VUC 2004.1 Linux ELF32 i386 archived executable.
Compiler authority: two independent recovered GCC 3.3.3 snapshots
(2004-01-25 and 2003-12-06) with GNU as 2.15.

## Mandatory preservation and rebase

Started on the existing Mac checkout `/path/to/revcl`
with `vuc` checked out, HEAD `e8912b8`, and **163 unrelated,
pre-existing untracked paths**. Fetched and rebased onto the newest
`origin/main`, replaying 137 `vuc` commits without conflicts.
No resets, cleans, dummy compiler functions, synthetic assembler
instructions, binary edits, original archive modifications or
additional worktrees.

The new work is confined to `vuc/`. The exact accepted
`vuc/src/Value.cpp` and its accepted related methods remain unchanged.

## Previously unresolved original function, now a strict partial

| Evidence | Independently verified result |
| --- | --- |
| Name | `Value::Set(ASTConstValExpr const*)` |
| Mangled symbol | `_ZN5Value3SetEPK15ASTConstValExpr` |
| Original address | **`0x0806e7d0`** |
| Original size | **455 instruction bytes** |
| Recompiled candidate size | **455 instruction bytes** |
| Matching original-address instruction bytes | **67/455** |
| Authenticated function relocations | **13** |
| Original named literal bytes | **11**, both actual `true\0` and `false\0` |
| Independent GCC snapshot reproduction | January 2004 **and** December 2003 |
| Status | **EXPERIMENTAL** — not byte exact |

Source:
`vuc/probes/ValueConstValSetPartialOct15b.cpp`

Selection metadata:
`vuc/config/value_set_constval_partial_oct15b.json`

Original-length compiler configuration:

```
-O2 -frename-registers -falign-jumps=4 -falign-labels=2 \
-falign-loops=1 -fno-reorder-blocks
```

**Do not confuse matching original function length with byte-exactness.**
The first nonmatching byte is at relative **+5** (the compiled
stack-frame size), and most downstream source-control-block bytes still
differ. The original emitted stack frame is `0x88`; the candidate's
frame is `0xb8`. The source is a period-plausible behavioral
reconstruction, not authenticated original C++ source.

## Real ABI and call graph recovered from the original

The original function reads and copies four contiguous 32-bit
fields from its `ASTConstValExpr` argument:
`+0x0c,+0x10,+0x14,+0x18`, writing them into `Value` at
`+0x00,+0x04,+0x08,+0x0c`.
The original expression string object is at AST argument offset
`+0x1c`. These are **grounded original i386 access offsets**,
not guessed field names.

The original dispatch handles distinct constant kinds:

- **Kind 1:** integer from `MLGameString::c_str()` through
  `__strtol_internal(..., 10, 0)`.
- **Kind 2:** boolean expressions checked against the actual
  original `"true"` and `"false"` literals, with integer parse
  fallback.
- **Kind 3:** scalar floating point parsed through
  `__strtod_internal(..., 0)` and converted to `float`.
- **Kind 4:** four float components; a bounded scratch buffer,
  `strrchr`, `memcpy`, NUL termination, three float parses
  inside the loop and a final float parse.

The real original conversion / libc locations are independently
authenticated in the manifest:

```
MLGameString::c_str()    0x08071800
__strtol_internal        0x0804984c
__strtod_internal        0x080495fc
strrchr                  0x080498dc
memcpy                   0x080498ac
"true\0"                0x08075f78    5 bytes
"false\0"               0x0807668a    6 bytes
```

The strict comparator resolves **all 13 candidate-emitted relocations** and checks
all **11 original literal bytes**. No external call or literal
address is fabricated. The method's exact type layout, Boolean
shortcuts and vector string behavior remain provisional: no
runtime behavioral-equivalence claim is made merely from the
byte comparison.

**Later same-day reference-count clarification:** these 13 resolved
relocations are the selected candidate's own emitted references, **not**
the complete original function's reference-site inventory. Directly reading
the archived ELF proves that the original has **13 relative CALL sites**
and **two separate Boolean literal-address sites** (15 references total).
The selected 455-byte C++03 candidate shares its Boolean fallback with the
integer case, eliminating one `c_str()` call and one `__strtol_internal()`
call. It therefore has only 11 CALL sites plus two literal references.
The separately verified 459-byte diagnostic source retains all 15 original
references but fails the original-size acceptance requirement. See
`vuc/docs/2026-10-15-vuc-set-15-reference-callgraph-audit.md` and the
independent original-ELF regression tests.

## Source/optimizer evidence (not promoted unless valid)

Nine new reproducible drivers capture 3,112 configurations:

| Driver in `vuc/probes/` | Compiler/source configurations | Strict outcome |
| --- | ---: | --- |
| `evaluate_value_plus_branch_ownership_oct15b.py` | 540 | `Value::operator+` remains **203/219** |
| `evaluate_division_lane_order_oct15b.py` | 480 | `Value::operator/` remains **178/235** |
| `evaluate_value_plus_compiler_passes_oct15b.py` | 416 | Both GCC snapshots; addition still **203/219** |
| `evaluate_division_compiler_passes_oct15b.py` | 444 | No genuine 235-byte candidate improves division; 237-byte candidate overlapping 212 is invalid |
| `evaluate_value_set_ast_abi_oct15b.py` | 378 | First source layout and real glibc ABI; candidates around **425–515** bytes, no 455-byte match |
| `evaluate_value_set_453_alignment_oct15b.py` | 180 | Best compiler candidates **454 bytes**, not acceptable |
| `evaluate_value_set_454_compiler_controls_oct15b.py` | 82 | **First 455-byte authenticated source**, 67/455 and 13 relocations |
| `evaluate_value_set_case_order_oct15b.py` | 240 | All 24 case lexical permutations; no improvement beyond **67/455** |
| `evaluate_value_set_lifetimes_oct15b.py` | 352 | Eight Boolean-string pointer lifetime forms × eleven vector parsing scope/loop choices × four compiler controls; 56 original-sized candidates retain **67/455** and 13 relocations, no improvement |

The ordinary source-level improvement came from expressing
**two independently retrieved string pointers**, preserving the
original's repeated `c_str()` calls and avoiding an earlier
temporary's lifetime across both Boolean comparisons.
The first parser ABI draft reached 438 bytes with the public
`strtol/strtod` calls; reproducing the original internal glibc
ABI exposed bodies around 425–515 bytes. Original-sized 455
arose only when testing the historical `-fno-reorder-blocks`
compiler control on the 454-byte configuration.

The result retains the original *function size*, but not the
original control-flow placement: no claim is made that
`-fno-reorder-blocks` is the historical compiler flag.
All wrong-length candidates are **excluded from accepted partials**,
even when their raw overlapping byte count is higher.

## Reproduce final accepted-constraint partial

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -falign-jumps=4 \
    -falign-labels=2 -falign-loops=1 -fno-reorder-blocks \
    -c "$R/probes/ValueConstValSetPartialOct15b.cpp" \
    -o "/tmp/vuc-binutils215/valueconst-selected-${snap}.o"
  limactl shell revcl-oracle -- python3 \
    "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/valueconst-selected-${snap}.o" \
    --config "$R/config/value_set_constval_partial_oct15b.json"
done
```

Each historical compiler independently outputs:

```
length=455
exact_bytes=67
relocations=13
verified_literal_bytes=11
exact=false
```

The ledger now includes all **298 original named functions**:
**200 EXACT / 25 EXPERIMENTAL / 73 UNRESOLVED**. This is
the *first strictly original-sized experimental reconstruction* of
`Value::Set(ASTConstValExpr const*)`, which was previously
UNRESOLVED; it is not a new exact function and does not increase
the 14,930-byte exact accepted total.

The unchanged other strict acceptance gates remain:
**200/200 exact functions** and **14,930/14,930** code bytes,
**244/244 initialized data objects** and **71,536 bytes**,
**12/12 BSS declarations** and **18,437 allocation bytes**.
The original linked ELF's whole-binary byte identity remains
**unavailable/unproven**.

## Follow-up

Improve the source to recover the original stack frame
`sub $0x88, %esp` instead of the diagnostic candidate
`sub $0xb8, %esp`; recover the original high-probability early
return and late Boolean-case block. Concentrate on live pointer /
scratch-buffer ownership, not meaningless alignment padding.
Run a full original-length / relocation / literal comparison on
each hypothesized source.

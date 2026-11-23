# VUC 2004.1 — Two newly byte-exact MLTempString C2 constructors

**Date:** 2026-11-23 · **Branch:** `vuc`, never merge into `main`
until the complete rebuilt VUC executable matches the original.

## Evidence and reason this succeeded

Both Itanium C++ ABI entry points, C1 and C2, are real independently
named functions in the original VUC 2004.1 ELF32 executable. Before
this investigation the **default C2** at `0x08071c20` was **95/101**,
and the **formatted C2** at `0x08071b40` was **98/110**. Their C1
siblings were already byte-exact.

The earlier isolated source forms and compiler-flag changes could not
recover the missing register allocation. Inspired by the successful VCL
reconstruction technique, we compiled **genuinely related 2004-era
C++03 member definitions in the same original-style source unit**:
the default constructor, variadic formatted constructor, and
`MLTempString::Set`. The prototype `void Set(const char*, ...)` matches
the existing provisional `MLGameString.h` contract; no artificial
helper calls, handwritten assembly, register pinning, dummy instructions
or altered vendor binary were introduced.

The **two accepted owners use different source orders and compiler flags**.
Each is a real C++ source file and compiles with **both** recovered
GCC 3.3.3 snapshots (2004-01-25 and 2003-12-06), GNU as 2.15:

| Newly accepted function | Original ELF address | Original length | Previously | Now | Original relocations | Compiler | Source order |
| --- | --- | ---: | ---: | ---: | ---: | --- | --- |
| `MLTempString::MLTempString()` C2 | `0x08071c20` | 101 | 95/101 | **101/101** | 9/9 | `-O2 -frename-registers` | formatted C2 → Set → default C2 |
| `MLTempString::MLTempString(const char*, ...)` C2 | `0x08071b40` | 110 | 98/110 | **110/110** | 8/8 | `-O2 -frename-registers -falign-jumps=1` | default C2 → Set → formatted C2 |

The original empty-string literal in default C2 is verified against
the vendor ELF (`0x08072286`). Both singleton BSS relocation objects
are independently mapped, not guessed. The formatted C2 has no literal
blob of its own and all eight call/data/BSS relocations are verified.

Both new owner units also compile their respective already-accepted
**C1 sibling exactly in the same object**:

- Default owner: C2 **101/101**, C1 **102/102**, nine relocations
  and original one-byte empty literal in each.
- Formatted owner: C2 **110/110**, C1 **110/110**, eight relocations
  in each.

These are **two new independently exact symbols (211 new exact bytes)**,
not claims of four additional matches. The C1 functions were counted
previously, and original function coverage counts unique function
symbols, not duplicate source-code body bytes.

## Source owner limitations

The accepted historical match-only files are:

- `vuc/src/MLTempStringDefaultC2Owner.cpp`
- `vuc/src/MLTempStringFormattedC2Owner.cpp`

These contain overlapping genuine constructor and Set definitions
as historical compiler-emission context. **The other methods in each
owner are NOT all exact from that owner**: each still compiles one
nonaccepted Set and an alternate constructor whose size can differ
from the original. Therefore these are independent **function-owner
research units**, not two objects to link into a finished executable.
They are intentionally excluded from modern CMake's 89-source VUC
object-library target and tracked separately as historical match units.
Deduplicating their source definitions and reconstructing the authentic
joint original C++ translation unit and final link remain future work.

The 144-recipe source-order matrix is reproducible with:

```sh
limactl shell revcl-oracle -- python3 \
  vuc/probes/evaluate_mltemp_real_tu_20261123.py
```

It compares three genuine source methods in all six orders, three
possible Set return-contract hypotheses and four historical compiler
flag sets, separately for both recovered historical compiler snapshots.
The winning `void Set` return contract is consistent with the accepted
C++ header. The source/object/results ledger is in the Linux guest
under `/tmp/vuc-mltemp-tus-20261123/`, not committed.

Reproduce the strict original-ELF match *after registration*:

```sh
limactl shell revcl-oracle -- python3 \
  vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-exact-209-20261123
```

The exact manifest now includes both new C2 symbols; their old
`partial_byte_checkpoints.json` floors are removed only after
promotion. No unrelated partial, original source data/BSS or VCL
source is modified. Full binary equality remains unavailable.

## Updated function coverage

- **209 exact** functions, **16,934 exact instruction bytes** (previously
  207 / 16,723).
- **35 experimental**, including **34 authenticated original-sized
  partials** and one unverified wrong-sized candidate; unchanged
  remaining partial scores total **6,990/10,923** instruction bytes
  *within those 34 functions only*.
- **54 unresolved** named symbols, including six zero-sized.
- **298 named text symbols**, original complete ELF still **314,338
  bytes**, no complete rebuilt VUC ELF or whole-file SHA match.

See the generated [full function score inventory](FUNCTION_BYTE_MATCH_STATUS.md).

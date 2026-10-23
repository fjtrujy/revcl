# VUC 2026-10-23: authentic Printer owner and GCC 3.3.3 vector scheduler forensics

Target: unchanged VUC 2004.1 Linux i386 executable embedded in the
tracked upstream ZIP. Branch: `vuc`, entry commit `e20bff5c`.
Investigations are **function-scoped** original-offset ELF comparisons,
not a whole-executable reconstruction. Source-only and historical-compiler
diagnostic controls do **not** authorize changing production compiler flags,
installed headers, hand-assembling registers, or relaxing exact acceptance.

## 1. Printer.cpp: first application body and its real GCC register web

Original `Printer.cpp` source provenance is recorded in
`vuc/config/source_units.tsv`; original ordered ELF function symbols:

| Original VA | Symbol | Status |
| --- | --- | --- |
| `0x0805b720` | `__static_initialization_and_destruction_0(int,int)` | iostream compiler helper |
| `0x0805b770` | `Print(ASTFunCall*,std::ostream&)` | **96/98** accepted experimental in plain `-O2` |
| `0x0805b7e0` | `Print(ASTStmt*,std::ostream&)` | long incomplete body, 2231 bytes |
| `0x0805c0a0` | `Print(ASTExpr*,std::ostream&)` | long incomplete body, 1089 bytes |
| `0x0805c4f0` | `Print(ASTLValExpr*,std::ostream&)` | long incomplete body, 765 bytes |
| `0x0805c7f0` | `Print(ASTDecl*,std::ostream&)` | long incomplete body, 844 bytes |
| `0x0805cb40` | `Printer_UglyPrint(ASTModule*,std::ostream&)` | **40/40** independently exact |
| `0x0805cb80` | `_GLOBAL__I__Z5PrintP10ASTFunCallRSo` | **31/31** independently exact |
| `0x0805cba0..` | AST expression/statement/decl list template instantiations | exact in separate manifest |

The unresolved AST printers are **not** represented by invented dummy
methods. Source compilation order and generated iostream initialization
context differ from bare symbol order, so ELF ordering alone is not proof
of frontend pass order.

The original 98-byte function loads the `ASTFunCall::callee` pointer
from `[ebx+8]` to **ECX** and copies it to the argument area before
the first genuine `Print(ASTLValExpr*,std::ostream&)` call.
Current recovered GCC3.3.3 does the identical operations via **EAX**:
only original offsets **+23** and **+29** differ at the best score
(**96/98**, seven authentic relocation sites, nine verified literal bytes).
The previous October 22 campaign established that the register choice
is already present in the **`.24.greg`** RTL insns **20/21**: the
actual callee pseudo maps to `reg:SI 0 eax`. Later renaming/scheduling
do not introduce the mismatch.

New reproducer:
`vuc/probes/evaluate_printer_original_tu_rtl_oct23.py`.

It compiles the genuine recovered `Printer.cpp` bodies, optionally
placing the **real exact `Print(ASTNodeList<T>)` template**
instantiations before/after the printer, under the two independently
recovered GCC 3.3.3 prerelease snapshots and three natural compiler
configurations: plain `-O2`, existing startup-exact
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`,
and `-O2 -fuse-cxa-atexit -ffunction-sections` without rename.

**18 historical compilations** with `-da`. Every variant's
`.24.greg` chooses **EAX** for the real first-callee field load.
All full-recoverable-owner results follow the same pattern:

| Flags | ASTFunCall | UglyPrint | Startup | ASTExpr list (when instantiated) |
| --- | ---: | ---: | ---: | ---: |
| `-O2` | **96/98** | 33/40 | 27/31 | 74/80 |
| `-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections` | 92/98 | **40/40 exact** | **31/31 exact** | **80/80 exact** |
| `-O2 -fuse-cxa-atexit -ffunction-sections` | **96/98** | 33/40 | 29/31 | 74/80 |

The real printer source may still contain unknown declarations, the four
long application printer bodies, or preprocessing details that change
compiler behavior; this is a **recoverable true-owner subset**, not a
claim that the entire original TU has been recreated. Neither simply
moving the genuine template instantiations nor using already-accepted
startup flags produces a valid simultaneous exactness improvement.
All scored objects and 18 source-unit RTL traces are in guest scratch
`/tmp/vuc-printer-original-owner-rtl-20261023/`.

## 2. Historical libstdc++: five relevant headers checked byte-for-byte

Compared **three independent retained copies**:

1. GCC 3.3.3 branch snapshot `2003-12-06`, from recovered source.
2. GCC 3.3.3 branch snapshot `2004-01-25`, from recovered source.
3. Compatibility libstdc++ headers originally installed with GCC 3.3.5.

Their files are **byte-identical for all five checked headers**:

| Header | SHA-256 |
| --- | --- |
| `bits/vector.tcc` | `53666ca1710aacfc4e20e54e45ed3591d5758ec7eee5b66cdf01a0b4a8eb44e6` |
| `bits/stl_vector.h` | `de0a28ad9b43b1952f5eb29adfc7d59d6fbe607cf1d8f1435be240bce462fba5` |
| `bits/stl_construct.h` | `6970c64a416d9a5048f75caed1028c2264bf9d8ccde14941d840c138093281d7` |
| `bits/stl_uninitialized.h` | `f9b0205b994016afd4570fb18bb0144acf213b31dcb23e9ae1748d61a59342a0` |
| `bits/stl_algobase.h` | `74ca5b0068240e48278ffb95c581e1e08c52f2b3c754f823588e2d716252b9` |

Specifically, the `vector.tcc` source already sequences the real
`_M_start = __new_start.base()` before the real
`_M_finish = __new_finish.base()`, followed by the capacity assignment.
The original VUC binary uses **start then finish**, but GCC with
the reconstructed original compiler outputs **finish then start**.
Thus no one of these three recovered source versions explains the
difference; this does **not** rule out an unidentified Debian patch
or compiler-code-generation difference.

## 3. Exact stage where vector success stores reverse

Existing authentic vector insertion:
`std::vector<SymbolScope>::_M_insert_aux`,
original **0x0806a550**, **746 bytes**;
**733/746** exact original-offset instruction bytes,
**all 15** genuine relocations;
all **13** mismatch offsets
`+513,+520,+534,+535,+540..+548`.

`vuc/probes/ScopeVectorInsertionOriginalUnitOct15i.cpp` produces this
weak template naturally from the already byte-exact
`SymbolTable::BeginScope` (**140/140**).
Inspecting recovered-Jan25 compiler `-da` dumps from the actual owner
isolates **BB35**:
- `.23.lreg`, `.24.greg`, `.26.flow2`, `.27.peephole2`,
  `.28.rnreg`: store **insn 1779** (`_M_start`) precedes store
  **insn 1797** (`_M_finish`), followed by the capacity store
  **insn 1820** (`_M_end_of_storage`).
- `.30.sched2`: the same original RTL insn identities reverse:
  **1797 before 1779 before 1820**. This proves post-register
  allocation instruction scheduling, not C++ lexical ordering,
  is the first pass that introduces the wrong success-store sequence.

Bounded genuine whole-owner scheduler controls:
`vuc/probes/evaluate_vector_sched_pass_controls_oct23.py`.
Eleven option choices under each recovered compiler (**22 builds**):
`-fno-schedule-insns`, `-fno-schedule-insns2`,
`-fno-sched-spec`, `-fno-sched-interblock`,
`-fno-sched-spec-load`, speculative-load variants,
`-fno-gcse`, `-fno-rename-registers`, and baseline.

**Every** candidate retaining `SymbolTable::BeginScope` 140/140
and original-size vector with a useful score is still 733/746
at the same 13 offsets.
- Disabling the *first* scheduling pass, inter-block scheduling,
  speculative scheduling or relevant speculative load controls
  has **no change**.
- `-fno-schedule-insns2` keeps the original 746-byte vector length
  but crashes the original byte match down to **370/746** and
  regresses `BeginScope`, so no production flag change is justified.
- `-fno-rename-registers` gives **561/746** while preserving the
  exact `BeginScope`; changing the global register-renamer option is
  not an acceptable fix.

Reproducer with historical i386 tuning controls:
`vuc/probes/evaluate_printer_vector_historical_cpu_context_oct23.py`,
tests 16 models/flag sets × two owners × both snapshots
(**64 attempted builds**). On surviving correctly sized/relocated
objects that protect already exact sibling functions, the best
vector score remains **733/746** and the best printer remains
**96/98**. GCC3.3 rejects `-mtune` as an unsupported option;
several `-mcpu`/`-march` models change code size and therefore
cannot be considered strict partials. On the original-size, sibling-
protected subset, `-mcpu=pentiumpro` and `-mcpu=pentium3`
reproduce the same vector mismatch.

Guest scratch score ledgers and compiled originals:
`/tmp/vuc-vector-scheduler-controls-20261023/`,
`/tmp/vuc-sched-target-cpu-20261023/`.
These are **compiler diagnostics**, not evidence authenticating original
production build flags.

## 4. Outcome and next best discriminator

**104 attempted original-compiler comparisons** in the tracked
diagnostic reproducer batch (18 Printer-TU + 22 scheduler + 64 tuning;
some unsupported flag sets fail compilation). Additional baseline
diagnostic `-fno-schedule-insns2` test confirms the same behavior
under both snapshots. The original archived ELF, existing root VCL
project, actual compiler binaries/headers, exact candidates and
production VUC sources were **not modified**.

No newly byte-exact function or improved admissible original-length
partial was found. Accepted baseline remains **205/205 exact functions,
16,093/16,093 checked instruction bytes**; separate initialized-data,
BSS and whole-ELF acceptance are not conflated with function coverage.

The remaining concrete research questions are now sharply defined:
1. Why did the original GCC/possible Debian build keep EAX
   unavailable or favor ECX for `ASTFunCall::callee` in the first
   Printer application body? Recovering the actual AST printers and
   original-era class definitions may change real source-unit
   optimization context; repeating cosmetic callee aliases does not.
2. Why does original VUC's scheduler preserve start-before-finish
   in vector growth while *both* recovered compilers reorder it in
   BB35? Compare the original Debian GCC `0pre3` backend/scheduler
   patches or first-vs-second-pass scheduling context, rather than
   changing the already identical `vector.tcc` sources or bypassing
   `sched2`.
3. Any source or toolchain hypothesis must retain real original
   function length, relocations, literal ownership, exact siblings
   and original-offset matches, with no assembler/register hacks.

Reproduce (the toolchain aliases must already be restored):

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_printer_original_tu_rtl_oct23.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_vector_sched_pass_controls_oct23.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_printer_vector_historical_cpu_context_oct23.py"
limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-exact-oct23
python3 -m unittest discover -s vuc/tests -q
```

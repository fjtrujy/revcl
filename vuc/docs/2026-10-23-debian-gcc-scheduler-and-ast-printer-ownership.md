# VUC 2004.1: original AST Printer ownership and Debian GCC scheduler provenance

**Date:** 2026-10-23. Branch `vuc`. Entry commit `3251ed61`.
Target: immutable 314,338-byte VUC 2004.1 Linux i386 executable inside
`archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip`;
ELF SHA-256 `14c77997057f279ab25a3591c6a379d6e4744f2b1551271d43602948c037d8cf`.
No new worktree, production compiler changes, source-byte padding, handwritten
assembler, or original binary edits.

## 1. Original Debian 0pre3 provenance vs. currently recovered toolchain

The original ELF includes two GCC signatures:
- `GCC: (GNU) 3.3.3 20031206 (prerelease) (Debian)`
- `GCC: (GNU) 3.3.3 20040125 (prerelease) (Debian)`

The source package contemporary to the latter tag is
`gcc-3.3` **1:3.3.3ds3-0pre3** (accepted January 26, 2004).
Debian's original accepted-source record names:
- `gcc-3.3_3.3.3ds3-0pre3.dsc`, MD5
  `5a6d5efd27f7312a7765d11051315509`
- `gcc-3.3_3.3.3ds3.orig.tar.gz`, MD5
  `04cfa8adeb1986e9c2fecb246c36068a`
- `gcc-3.3_3.3.3ds3-0pre3.diff.gz`, **2,265,647 bytes**, MD5
  `67e4f8e4a184ed8c811e78b82ef3e9c6`.

Contemporary primary evidence:
- https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=195913
- https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=212912

The Debian change record mentions GCC 2004-01-25 CVS plus fixes PR11350,
PR11793, PR13544/PR12862, and PR12441, as well as Debian
packaging/architecture changes. **These descriptions do not establish
which i386 scheduler/register allocator changes the original patch did
or did not contain.** The exact `0pre3` source diff and original
compiler binaries were **not recovered**: old Debian pool and dated
snapshot archive URLs have already proved unavailable, as previously
documented in `vuc/docs/toolchain-recovery-2026-10-10.md`.
A direct historical `snapshot.debian.org` archive URL checked this
session also returned HTTP 404. No other package/version is presented
as authenticated `0pre3`.

The current usable compiler is the independently reconstructed upstream
GCC3.3.3 2003-12-06/2004-01-25 source snapshots with GNU as 2.15.
Its old libstdc++ headers are identical across five checked vector and
STL support files to the archived GCC3.3.5 compatibility copy, as
established in `vuc/docs/2026-10-23-printer-owner-and-vector-sched2-forensics.md`.
Thus the **live lead** is the still-unrecovered Debian source patch
and scheduler/backend provenance, not arbitrary `vector.tcc`
reordering.

## 2. The exact vector store scheduling dependency

Original `std::vector<SymbolScope>::_M_insert_aux`:
`0x0806a550`, **746 bytes**, accepted experimental **733/746**
matching bytes, **15 real relocations**. The 13 residual differing
original offsets remain `+513,+520,+534,+535,+540..+548`, entirely
inside the successful vector reallocation commit.

The earlier pass-level comparison found:
- original `vector.tcc`: `_M_start` before `_M_finish`;
- `.28.rnreg`: store RTL **1779** (`_M_start`) before **1797**
  (`_M_finish`);
- `.30.sched2`: **1797** before **1779**, followed by **1820**
  (`_M_end_of_storage`).

A new **`-fsched-verbose=9`** original-owner reproduction isolates
the *mechanism*, not just the reordered instruction sequence.
Object/RTL logs remain outside Git at
`/tmp/vuc-20261023-sched-detail/` in the Linux guest.

In original growth-commit **BB35**, the scheduler's actual ready
queue and simulated Pentium Pro execution pipeline show:

| Event | Original GCC RTL insn | Scheduler clock |
| --- | ---: | ---: |
| Load pointer to new finish into ECX | 2134 | 2 |
| Load pointer to new start into EDX | 2133 | 4 |
| Finish store `[esi+4]=ecx` becomes scheduled | 1797 | **5** |
| Start store `[esi]=edx` becomes scheduled | 1779 | **7** |
| Capacity store `[esi+8]=ecx` | 1820 | 8 |

The scheduler explicitly reports **3-cycle readiness delay**
between `2134 -> 1797` and `2133 -> 1779`. The finish load
is scheduled two clocks earlier than the start load, so its store is
ready two clocks earlier. This explains why neither rearranging
the source's three stores nor changing trivial pointer aliases
recovers the original. **The genuine earlier load dependencies and
their priority must change** (or the exact historical compiler
scheduling implementation must be recovered).

This observation does not authorize changing `-O2`, disabling
`.sched2`, patching the scheduler, or forcing hard registers:
the previous whole-owner control dropped to 370/746 and regressed
an already exact `SymbolTable::BeginScope`.

Reproduce from the restored historical VM:

```sh
R=/path/to/revcl/vuc
mkdir -p /tmp/vuc-20261023-sched-detail
cd /tmp/vuc-20261023-sched-detail
/tmp/vuc-binutils215/g++-jan25-as215 -O2 -frename-registers \
  -falign-jumps=1 -fsched-verbose=9 -da -c \
  "$R/probes/ScopeVectorInsertionOriginalUnitOct15i.cpp" -o probe.o
# Inspect the BB35 ready list in ScopeVectorInsertionOriginalUnitOct15i.cpp.30.sched2.
```

## 3. Real original AST printer class and recursive-match-stack layout

Recovered directly and reproducibly from the pinned original ELF via
`vuc/probes/inspect_ast_printer_dispatch_oct23.py`, rather
than by guessing from current AST provisional structs.

| Printer | Original VA | Function size | Selector | Cases |
| --- | --- | ---: | --- | --- |
| `ASTStmt*` | `0x0805b7e0` | 2231 | signed int at **+8** | -1 through 8, 10 jump-table entries |
| `ASTExpr*` | `0x0805c0a0` | 1089 | signed int at **+8** | -1 through 4, 6 jump-table entries |
| `ASTDecl*` | `0x0805c7f0` | 844 | signed int at **+8** | -1 through 3, 5 jump-table entries |
| `ASTLValExpr*` | `0x0805c4f0` | 765 | signed int at **+0x1c** | 0, 1, 2; conditional dispatch |

The first three dispatch by the **selector plus one** using original
jump tables at `0x08072fb0`, `0x08072fd8`, and
`0x08072ff0`. The extractor verifies each table entry is
inside the corresponding exact original function's byte span.
Case -1 maps to the relevant ordinary epilogue; no assertion that
-1 is a valid user-visible runtime enum is made.

All four functions use the exact same **12-byte BSS**
`matchStack` at `0x0808abe4`. Earlier `AST.cpp` exact
initialization evidence proved it was zero-initialized as three
words but left semantics provisional. The printer **reads/writes
these fields as**:
- `matchStack + 0`: signed current node count;
- `matchStack + 4`: signed allocated capacity, initially zero;
- `matchStack + 8`: pointer to a 4-byte-pointer array.

Each case pushes a node. When count reaches capacity, the inlined
container growth uses `new[]`, **16** slots on first allocation or
**2×** the previous capacity thereafter, and copies existing
pointers via GCC 3.3's `rep movsl` sequence. It calls its child
printer(s), then decrements the depth if positive. The exception
landing pad also decreases the depth before `_Unwind_Resume`.
This repeated logic accounts for much of the four printers'
large machine-code bodies and should be reconstructed as one
genuine C++03 container and RAII scope guard rather than
four independent handwritten assembly helpers.

### First source-directed ASTLValExpr candidate (non-admissible yet)

New `vuc/probes/PrintASTLValRecursionOwnerOct23.cpp` models
three source-level AST variants with the original field offsets:
- selector 0: identifier, `MLGameString` at `+0x24`,
  `MLGameString::c_str()`;
- selector 1: indexed l-value, LVal pointer at `+0x24`,
  `ASTExpr*` index at `+0x28`, prints `[` and `]`;
- selector 2: member l-value, two `ASTLValExpr*` operands at
  `+0x24` and `+0x28`, prints `.`.

These offsets, direct call targets, and literal contents are observed
in original code; names and particular C++03 class declarations
remain **source hypotheses**. The case-local `ScopedPrintNode`
adds the parent to `matchStack` and ensures normal/exception
cleanup without writing synthetic machine code.

Both recovered GCC 3.3.3 snapshots compile this natural model at
`-O2` to **722 bytes**, against the original **765**. This
is a **size comparison, not a 722/765 byte-match score**.
The source also preserves the original ELF RTTI chain
`ASTLValExpr` → `ASTExpr` → `ASTNode`: the provisional
`ASTNode` has a vtable pointer and line field (8 bytes), the
`ASTExpr` selector lives at +8, four additional unknown words
follow it, and `ASTLValExpr`'s own selector lives at +0x1c.
No original-length/relocation/literal-authenticated partial is
claimed; the source is **NOT accepted** and remains in
`vuc/probes`, not `vuc/src` or selected/exact candidate manifests.

`vuc/probes/evaluate_lval_case_order_oct23.py` compiles
different semantically equivalent case permutations as `switch`
and `if/else` control-flow forms under both historical compiler
snapshots. The complete **96-build** matrix includes
60 switch-case/flag variants and 36 if-chain/flag variants.
The generated source/object and JSON-size ledger stay in guest scratch
`/tmp/vuc-lval-real-cases-20261023/`.
These variants demonstrate which original-sized-source layout differences
are genuine and which are normalized: the best emitted code remains
**722 bytes**, not the original 765. No candidate is promoted.

This is an **experimental architectural reconstruction**, rather than
a byte-exact improvement. It makes the next research task concrete:
recover the original AST node data types and exact
`ASTNodeList::Add`/scope-guard C++ constructs that account for the
remaining stack frame, code-size, and register allocation differences.

## 4. Acceptance contract and next targets

Use the exact historical compiler only through the existing verified
scratch aliases. Do not touch root VCL source or modify unrelated
untracked files. A 765-byte function with every original call
relocation and literal confirmed is necessary before an ASTLValExpr
partial can be selected, and **all bytes** must match before promotion
to `vuc/src`.

Regression run with the original-ELF historical GCC comparator:
**205/205 exact functions, 16,093/16,093 instruction bytes**;
**244/244 named initialized-data objects, 71,536/71,536 bytes**;
**12/12 BSS declarations, 18,437 bytes of source allocation**.
The Python host suite passed **199 tests** (its separate deliberate
non-exact acceptance fixture remained rejected). These are separate
function/data/declaration gates, **not** a complete linked binary match.

For the Debian toolchain, only an authenticated `0pre3` diff or actual
original binary can distinguish Debian i386 backend patches from
version-matched upstream compiler output. Confirm identity against
the contemporary source MD5 before applying patches in isolated scratch.

For the printer, reuse the verified matchStack/AST dispatch extractor to
build realistic original `Print(ASTStmt*)`, `Print(ASTExpr*)`,
`Print(ASTDecl*)`, and `Print(ASTLValExpr*)` bodies, including
their genuine recursive calls, switch arms, no-op cases and
exception-safe stack-depth management. Avoid fake source
predecessors and arbitrary register constraints.

Reproduce:

```sh
python3 vuc/probes/inspect_ast_printer_dispatch_oct23.py
limactl shell revcl-oracle -- python3 /path/to/revcl/vuc/probes/evaluate_lval_case_order_oct23.py
python3 -m unittest discover -s vuc/tests -q
limactl shell revcl-oracle -- python3 /path/to/revcl/vuc/scripts/match_candidates.py --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-exact-20261023-ast-ownership
```

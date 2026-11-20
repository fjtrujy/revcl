# VUC: Scope EH landing-pad and AsmOutput return-contract controls (2026-11-20)

This report concerns only VUC 2004.1 Linux ELF32-i386 in the immutable
`archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip` member
`X86linux_vuc2004_1/vuc`. It does not use the VCL project as the oracle.
All experiments ran in the existing `revcl-oracle` Linux VM using the
recovered GCC 3.3.3 January 25, 2004 and December 6, 2003 prerelease
snapshots with GNU as 2.15, not authenticated original Debian binaries.
The isolated source checkout was `.revcl-vuc-rebase-20261120` at
entry commit `7d10a132`; no push or canonical `main` checkout edits.

## 1. Original Scope normal-iterator copy: identify the exact missing block

Original symbol: `std::__uninitialized_copy_aux<ScopeIter, ScopeIter>`,
`0x0806a840`, **149 original instruction bytes**. The authentic
`vuc/src/SymbolTableUnit.cpp` compilation under
`-O2 -frename-registers -falign-jumps=1` emits **145 bytes**, with
**all six supported original call relocations** resolved. The canonical
relocation-aware comparator independently confirmed **92 contiguous
identical original-offset instruction bytes** before the mismatch at
`+0x5c` (92). Because the emitted size is wrong, **92/145 is a
diagnostic prefix/overlap only, NOT a valid authenticated
original-length partial**.

Disassembly establishes that the original has a natural compiler/
assembler-generated four-byte NOP-equivalent alignment at
`0x0806a89c` through `0x0806a89f` (relative `+0x5c..+0x5f`).
The original exception catch entry is at `+0x60`, while the
same catch-entry instructions in the rebuilt candidate begin at
`+0x5c`. The original has the ordinary return epilogue beginning
at `+0x50`; that preceding code and the six-call exception graph
are already reconstructed. These facts localize the missing length to
the EH landing boundary rather than a missing source operation.

`-falign-jumps=1..8` alone yielded generated lengths
`145,145,147,147,151,151,151,151`. Holding jumps at 1 and using
`-falign-labels=1,2,4,8,16` yielded
`145,147,151,159,191`. Broad label alignment is therefore not a
valid substitute for the target's particular four-byte EH pad.
Previously documented much larger jump/label/loop matrices and
actual template/source-order controls remain negative; they were
reviewed rather than rerun.

### Causal i386 CPU-target control

New reproducer:
`vuc/probes/evaluate_scope_copy_cpu_20261120.py`.

It compiles **13 real i386 target configurations under both recovered
GCC snapshots (26 builds)**, always the genuine SymbolTable source
unit and the same six originally authenticated relocations, without
touching a compiler binary, global flags or accepted source:

| Target family | Generated helper length |
| --- | ---: |
| GCC default, i686/PentiumPro/Pentium3 via `-mcpu` or `-march` | 145 |
| i386 via `-mcpu` or `-march` | 146 |
| i486 or i586 via `-mcpu` or `-march` | 154 |

Both GCC snapshots agree on each result. **No generated helper has
the required original length 149**, so none is eligible for full
relocation/byte acceptance or source promotion. This rules out
those CPU model options **on their own** as the cause of the required
landing-pad alignment. It does not rule out the missing original
Debian GCC patch or a real altered template/control-flow predecessor.

All source/object and per-case ledger files were generated only in
VM scratch at `/tmp/vuc-scope-cpu-context-20261120/`; the baseline
disassembly and comparator object are under
`/tmp/vuc-next-investigate/`.

### Genuine STL point-of-instantiation ownership

The weak `__uninitialized_copy_aux` specialization may be instantiated
at a source location different from the order of strong SymbolTable
definitions. New reproducer:
`vuc/probes/evaluate_scope_instantiation_placement_20261120.py`.

Eight ordinary C++03 source-unit configurations keep the *same*
authentic SymbolTable methods, including the implicit template owner
and explicit STL instantiations before the SymbolTable class, before
each of Enter/Find/BeginScope/EndScope, before FindCopy and after
FindCopy. Both recovered GCC snapshots compiled each position:
**16 additional builds, all 145 bytes**, with the original 149-byte
requirement still unmet. Crucially, all **four independently accepted
neighboring functions** (`SymbolTable::Find`, `BeginScope`,
`EndScope`, and `MLHashTable::FindCopy`) remained byte-exact in
every object. These controls directly disprove simple explicit
template-instantiation placement as a source of the required four
landing-pad bytes. Full guest ledger and generated objects are in
`/tmp/vuc-scope-instantiation-placement-20261120/`.

## 2. AsmOutput_WritePrologue genuine return type

Original `AsmOutput_WritePrologue(std::ofstream&)` at
`0x08049b40`: **30 bytes**, accepted experimental **22/30**,
two original resolved call/data relocations, and **92 authenticated
literal bytes**. The C++ function's mangled name does not encode
its ordinary non-template return type. A historical source hypothesis
is that the author wrote an ostream-returning stream insertion
instead of a `void` statement.

New reproducer:
`vuc/probes/evaluate_prologue_return_contract_20261120.py`.

**Six normal C++03 return/receiver forms under each recovered GCC
snapshot (12 builds)** were compared with the original strict
function manifest:

- Original `void`, `std::ostream&`, `const std::ostream&`,
  and two natural void receiver aliases all emit the **original
  30-byte length** but remain **22/30**, with precisely the same
  mismatching offsets `+11,+12,+13,+14,+15,+16,+17,+19`,
  both relocations and all 92 original literal bytes.
- Returning `std::ofstream&` emits **38 bytes**, not the original
  30; it is structurally rejected, not scored as a partial.

The two compiler snapshots behave identically. Thus the real
ostream-return contract and basic receiver-lifetime alternatives
do **not** explain the original register/scheduling difference.
This does not establish that the original return type was void.

Reproduction ledger and compiled objects are in VM scratch:
`/tmp/vuc-prologue-return-contract-20261120/`.

## 3. Preservation, admission and next discriminator

No experiment in this report produced a new **original-length,
all-bytes-and-relocations exact function** or an improved
original-length authenticated partial. **Neither source fragment was
promoted into `vuc/src/` or an accepted manifest**. The original ZIP,
accepted C++ source, data/BSS evidence, compiler binaries and other
agent's VCL checkout remain unchanged.

The next genuinely discriminating work for `0x0806a840` is either
(a) an original historical compiler/assembler rule that selectively
aligns the EH landing entry without shifting the surrounding
normal-return and loop labels, or (b) a real C++03 STL exception
source/predecessor dependency that changes GCC's CFG/landing-pad
classification. Do not add source filler, inline ASM, hand-written
NOPs, assembler alignment directives, or an invented relocation
to manufacture the missing four bytes. Other supported targets
remain `Value::operator+` (203/219) and
`Print(ASTFunCall*,std::ostream&)` (96/98); their ordinary spelling
and alias sweeps were previously exhausted.

Independent full exact and partial regression runs **PASSED**:
**207/207 exact functions, 16,723/16,723 checked instruction bytes**
and **36 of 37 authenticated original-length experimental functions,
7,183/11,134 original-offset matched bytes**, with **zero partial
regressions**. The distinct original-data verification **PASSED at
244/244 objects and 71,536/71,536 bytes**; the separate BSS gate
**PASSED at 12/12 original symbol declarations, 18,437 bytes of
allocated storage (zero file-backed BSS bytes)**. The named-function
inventory is **207 EXACT / 37 EXPERIMENTAL / 54 UNRESOLVED**.
These are candidate-function measurements; a verified native
whole-ELF byte score is **UNAVAILABLE**.

Regression commands (the VM uses its isolated `/tmp/revcl-vuc-stage`
VUC snapshot, not the canonical checkout):

```sh
limactl shell revcl-oracle -- python3 \
  /tmp/revcl-vuc-stage/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-next-exact-20261120
limactl shell revcl-oracle -- python3 \
  /tmp/revcl-vuc-stage/vuc/scripts/match_partial_checkpoints.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-next-partial-20261120
/path/to/revcl/.venv/bin/python \
  -m unittest discover -s vuc/tests -q
```

The host test run **PASSED 205/205**; its deliberately negative
acceptance-fixture subprocess printed `Candidate byte-exact
acceptance: FAIL`, which is expected for that test. The exact and
partial regression gate results above were taken from the actual
independently generated summaries rather than inferred from test
source. No new exact function or admissible partial was promoted.

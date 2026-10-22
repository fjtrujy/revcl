# 2026-10-22 VUC: transfer GCC register-web and source-unit lessons from VCL

Target: the unchanged VUC 2004.1 Linux i386 executable inside
`archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip`.
Branch: `vuc`, entry HEAD `0acbc6b0`.
This report describes **strict original-offset original-length** comparisons,
not a whole-ELF percentage. All generated objects and temporary historical
libstdc++ header copies remain outside Git, under `/tmp/` in `revcl-oracle`.

## Compiler provenance and preservation

Unlike VCL's Red Hat GCC 3.2.2, VUC requires reconstructed GCC **3.3.3
prerelease** snapshots of **2003-12-06** and **2004-01-25**, plus GNU
`as` **2.15**. These are historically compatible recovered candidates,
*not authenticated exact original Debian binaries*. The preserved Linux
guest had been stopped and its `/tmp` scratch paths cleared; resumed it
and restored the existing persistent compiler aliases using
`vuc/scripts/restore_cached_toolchain.sh`. No original archives,
compiler source, tracked VCL files, or accepted VUC C++ fragments changed.

VCL's applicable lessons were: (1) investigate the *real* RTL
definition/use chain at its first disagreement; (2) genuine values live
across calls can change GCC register coloring, unlike mere aliases;
(3) distinguish the allocator's choices from later register-renaming;
(4) normal-looking, equivalent C++ expression/loop spelling is often
canonicalized before register allocation; and (5) protect original
relocations, literal contents, exact source-unit siblings, and
pre-existing accepted code when measuring a purported improvement.

## Print(ASTFunCall*, std::ostream&): two wrong register bytes

Original: `_Z5PrintP10ASTFunCallRSo`, **0x0805b770**, **98 bytes**.
The original loads the callee through `mov ecx,[ebx+8]` and moves ECX
into the first call's argument slot; the reconstructed source
`vuc/src/Printer.cpp` compiles to EAX for both. The two differences
are at offsets **+23** and **+29**, with **7 authentic relocations**,
**9 verified literal bytes**, and an original-length object.

A complete-owner Jan25 `-O2 -da` trace of the accepted experimental
`vuc/probes/PrintASTFunCallRealUnitOct15m.cpp` shows in
`.23.lreg` an operand chain using pseudo **61** for the
`ASTFunCall::callee` load. In **`.24.greg`**, insn **20** has already
allocated **EAX** (`reg:SI 0 eax [61]`) for the real field load, and
insn **21** copies EAX into `(esp)`; `.28.rnreg` and
`.30.sched2` leave it in EAX. The original ECX assignment therefore
requires a different earlier live/register-interference graph; a
late mnemonic or register-renaming change is not the cause in `-O2`.

New reproducible source-only driver:
`vuc/probes/evaluate_printer_regweb_crossing_oct22.py`.
It compiles **64** full original-shaped source-unit C++03 variants
under **each** recovered compiler (**128 builds**). Crossed genuinely
consumed callee receivers (direct member, named pointer, const pointer,
slot pointer, reference, whole-call reference or pointer, and field-byte
address) with direct/reference/pointer argument-list receivers and
declarations before/after the first real call. **All 64/64 per compiler**
have the original **98-byte** length and pass relocation/literal
validation; **36** score **96/98** with the original two mismatches,
**12** score 79/98, and **16** score 70/98. **No exact or improved
partial**. Code, objects and full JSON score ledger:
`/tmp/vuc-printer-arg-web-20261022/`.
The invariant across both independent GCC snapshots rules out this
particular source-lifetime crossing family.

## std::vector<SymbolScope>::_M_insert_aux: successful-commit dataflow

Original: **0x0806a550**, **746 bytes**, **15 relocated calls**.
Existing source `vuc/probes/ScopeVectorInsertionOriginalUnitOct15i.cpp`
genuinely instantiates the STL template through the already exact
`SymbolTable::BeginScope` (**140/140**). It has **733/746** exact
original-offset code bytes. The only 13 mismatches are at
**+513,+520,+534,+535,+540..+548**, inside the *successful*
reallocation commit. This is not an exception-path or linker issue.
The original writes vector `_M_start` then `_M_finish`;
the Jan25/Dec06 compiler's vector.tcc source also spells that order,
but the final machine code writes the finish pointer first.

The historically plausible hypothesis is that the original C++03
libstdc++ inline/template source had slightly different real
member-store dependencies or spelling from the available GCC 3.3.5
compatibility headers. The next two experiments copy **only** the
compatibility `bits/vector.tcc` into ignored guest scratch and
temporarily inject it with `-I`; neither the installed compiler,
library, original archive, nor any production source is edited.

- `vuc/probes/evaluate_scope_vector_store_order_oct22.py` tests all
  **6** orders of the three independent pointer assignments across
  **both** GCC snapshots (**12 builds**). Original declaration order
  `012` stays **733/746**. Order `021` yields **718/746** with
  original length. The other four orders each emit **740 bytes**,
  so fail the strict original-length gate. `BeginScope` remains
  **140/140** in all cases.
- `vuc/probes/evaluate_scope_vector_epilogue_lifetimes_oct22.py`
  tests **42** plausible alternative dataflow shapes per snapshot
  (**84 builds**): computing capacity from the newly stored
  `_M_start` instead of the iterator, naming the true cached start
  and finish pointers, late/early local value and reference lifetimes,
  const pointer values, and `this` member access. Variants capturing
  the *old* start as the new start are explicitly excluded because
  they would change semantics. **All 42/42 per snapshot** emit an
  original-length **733/746**, same 13 original-offset mismatches,
  with all authentic relocations and exact `BeginScope`.
  GCC canonicalizes these real dependencies into the accepted code.

Reproducible ledgers/sources/ELF objects:
`/tmp/vuc-vector-stores-oct22/` and
`/tmp/vuc-vector-epilogue-dataflow-oct22/`.
These experiments do not establish a different original
libstdc++ header version, and no header is promoted to accepted VUC.

## Baseline, conclusions, reproduction

**224 authentic original-compiler builds**:
128 printer + 12 STL statement orders + 84 STL lifetime controls.
All use two independently recovered GCC 3.3.3 prerelease snapshots
and GNU as 2.15. No positive strict original-byte improvement was
found. The accepted VUC count remains **205 exact / 39 experimental /
54 unresolved** among 298 named ELF function symbols; the
whole-linked-ELF byte score remains **UNAVAILABLE**.
The full recovered-Jan25 original-ELF exact acceptance suite passed:
**205/205 verified functions, 16,093/16,093 instruction bytes**.
The host Python suite also passed **199 tests** (its deliberate negative
acceptance fixture continued to reject a non-exact candidate).

Start the existing VM and, when `/tmp` aliases are absent, restore
from the pre-existing cached toolchain; **no rebuilding or replacements**
are necessary:

```sh
limactl start revcl-oracle
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- sh "$R/scripts/restore_cached_toolchain.sh"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_printer_regweb_crossing_oct22.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_scope_vector_store_order_oct22.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_scope_vector_epilogue_lifetimes_oct22.py"
limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" --compiler /tmp/vuc-binutils215/g++-jan25-as215 --out /tmp/vuc-20261022-exact-gate
python3 -m unittest discover -s vuc/tests -q
```

Next valuable discriminator: actual GCC3.3 `.24.greg` register
interference from a real previously unmodeled **source-unit-owned**
value in the AST printer, or the missing original object/template
version that affects vector success-block register scheduling.
Do not repeat equivalent pointer aliases or arbitrarily reorder
the three vector stores; neither produced progress. Preserve the
exact-only `vuc/src` policy and keep useful partials in
`vuc/probes` with relocation and literal checks.

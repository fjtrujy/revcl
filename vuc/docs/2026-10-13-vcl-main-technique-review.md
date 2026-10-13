# Reviewing VCL `main` matching techniques for VUC 2004.1

Date: 2026-10-13. Active branch: `vuc`.

## Rebase and documentation scope

Rebased all 75 existing VUC commits onto `main` at `06c3edd` (the
latest fetched `origin/main`), without conflicts. The checkout has no
tracked pending changes following the rebase; all 133 pre-existing
untracked files, including exploratory probes and extracted archives,
were preserved. The rebase rewrites VUC commit IDs by design.

Reviewed the full tracked Markdown corpus: **92 files**, comprising
**24 inherited from `main`** and **68 VUC-specific files**. The most
relevant VCL evidence is in `docs/matching-notes.md`,
`docs/decompilation.md`, `docs/workflow.md`, `AGENTS.md`, and
`config/matching-evidence/*.md`. The historical VUC-specific baseline
and restrictions remain in `vuc/AGENTS.md`,
`vuc/docs/exact-match-contract.md`, the toolchain reports and the
original-link-layout inventory.

**The VCL techniques are hypotheses, not VUC compiler facts.** The VCL
target uses GCC 3.2.2 (Red Hat 2003); VUC uses GCC 3.3.3 prerelease
signatures from December 2003 and January 2004. The source-date
January compiler and GNU as 2.15 are compatible reconstruction tools,
but the complete original patched Debian compiler, headers and linker
are still unauthenticated.

## Source/optimizer hypotheses worth testing

| Evidence from VCL `main` | Conditional application to VUC |
| --- | --- |
| Classify residuals by the first RTL pass where candidate behavior diverges; distinguish allocation, peephole2 scratch selection, register renaming and scheduling | Use *VUC's own* GCC 3.3.3 RTL dumps to locate the stage behind a near-match rather than repeatedly changing source or flags |
| GCC 3.2 peephole2 scratch searches can carry state across genuine definitions in one translation unit | Rebuild plausible real VUC source units, verify exact neighbors, and test if GCC 3.3.3 exhibits the same mechanism; never insert artificial predecessor functions |
| Lexical `switch` arm order and branch polarity can alter block identities, cold-label alignment and useful registers even with the same final CFG | Test only source orders consistent with original disassembly, and rerun the full exact manifest when changing any accepted TU |
| Scoped locals, direct member rereads versus cached pointers, signedness, and `bool`/byte/int distinctions alter allocation and instruction selection | Compare original field widths, load order and callee ABI first; preserve natural C++ semantics and original class layouts |
| Ordinary inline helpers or RAII scope boundaries can alter stack argument cleanup, exception paths and return normalization | Useful where VUC iostream/STL calls, C1/C2 clones, destructors, or varargs retain otherwise unexplained cleanup code |
| `#pragma interface` / `#pragma implementation` can control historically deferred inline copies | Investigate only when VUC's GCC 3.3.3 compiler and original symbol/section ordering support it; not a universal solution for clone mismatches |
| A source rearrangement can make isolated function bytes exact while perturbing `.comment`, literals, unwind and linker layout | Measure real objects, references, CIE/LSDA and eventually the *linked ELF*. Do not treat isolated exact functions as a whole-executable score |

The VCL main branch includes **user-authorized** exceptions for VCL
source-unit splitting and definition rearrangements (for example,
`cgreadfile-split.md`, `line-owner-split.md`,
`loc-tag-reconstruction.md`, and
`parser-buffer-reconstruction.md`). Those permissions and claims
are **not automatically transferable** to VUC. VUC still requires
ordinary source reconstruction, no fabricated code, no artificial
padding, preservation of known exact functions and all original
symbol/literal/relocation checks. A matching score alone does not
authenticate a compiler flag or a historical source-file boundary.

## Independently verified GCC 3.3.3 diagnostic differences

The recovered VUC compiler wrapper does **not** recognize
`-fdump-rtl-all`. The historical **`-da`** flag works and produces
the following VUC GCC 3.3.3 pass files for the ordinary
`vuc/probes/ugly_print.cpp` source:

- `ugly_print.cpp.24.greg`: global register allocation
- `ugly_print.cpp.26.flow2`: flow and stack cleanup
- `ugly_print.cpp.27.peephole2`: peephole / scratch selection
- `ugly_print.cpp.28.rnreg`: register renaming
- `ugly_print.cpp.30.sched2`: postreload scheduling

These are **not** the GCC 3.2.2 pass indices quoted in VCL's notes.

For `Printer_UglyPrint()` at original `0x0805cb40`, the recovered
source remains **38/40**, not exact. The candidate's own
`.27.peephole2` RTL uses **EAX** for the string-literal symbol
`*.LC0`. Its `.28.rnreg` RTL changes that literal load and
argument store to **ECX**. The original executable uses **EAX**
for that literal and store. This concretely places the
candidate's two-byte residual **at or after GCC 3.3.3 register
renaming**; it is not evidence that a peephole2 scratch-register
search alone caused the difference. The original executable does
not contain its own intermediate RTL, so the precise original
source/optimization cause remains unproven.

Reproduce without changing tracked source or binary archives:

```sh
limactl shell revcl-oracle -- /bin/sh -c '
  mkdir -p /tmp/vuc-binutils215/rtl-review-oct13
  cd /tmp/vuc-binutils215/rtl-review-oct13
  /tmp/vuc-binutils215/g++-jan25-as215 \
    -O2 -frename-registers -da -c \
    /path/to/revcl/vuc/probes/ugly_print.cpp \
    -o ugly_print.o
  grep -n -m 1 "symbol_ref/f:SI" ugly_print.cpp.27.peephole2
  grep -n -m 1 "symbol_ref/f:SI" ugly_print.cpp.28.rnreg
'
```

No historical compiler or assembler was rebuilt.

A bounded follow-up to the interrupted printer investigation also checked
**72 ordinary-source/flag combinations** with the recovered historical
compiler: 12 variants of member access, stream aliases and literal locals,
under six supported optimizer settings. The highest result remained
**38/40**, with differences at instruction-byte offsets **1 and 15**.
Plain `-O2` variants reached only 29/40, despite retaining the 40-byte
original size. The reproducible search is
`vuc/probes/evaluate_ugly_print_source_oct12.py`; **no variant is exact**.
This supports investigating source-unit/register-renamer context rather
than repeating equivalent local expression substitutions.

## VUC continuation priorities informed by this review

1. Diagnose the **first divergent pass** for `Canonize2_Reduce()`
   (83/85, original `0x0804ff50`), `Print(ASTFunCall*)` (96/98,
   `0x0805b770`), `script_error()` (155/161, `0x0806fa80`)
   and `Value::operator*()` (219/222, original symbol address in the
   manifest). Classify code-shape discrepancies separately from
   register-only discrepancies and source-unit effects.
2. For the `Instruction` arena cleanup (45/46 at `0x08051980`),
   verify whether the final pop register is a dead/peephole scratch
   choice, a renamed live value or a true source-level difference.
   GCC 3.2 results do not decide which pass is responsible in VUC.
3. Reconstruct actual VUC translation-unit contexts using original
   `STT_FILE` names, symbols, local scopes, iostream initialization,
   function ordering and compiler-generated C1/C2 and cleanup code.
   Do not infer missing VUC C++ structures from DWARF: the available
   DWARF belongs to glibc startup units.
4. Keep investigating historical GNU STL/iostream ABI and legitimate
   source order where C1/C2 clones disagree. Preserve exact variants
   and use scratch objects for all unaccepted alternatives.
5. Continue the independent linker/layout work: original section
   ordering, CRT/libstdc++ provenance, dynamic relocations, complete
   startup object emission and all padding still block entire-ELF
   identity. The canonical ZIP member is the sole final acceptance
   oracle.

## Revalidated post-rebase baseline

- Host: **119 unit tests passing**.
- Strict function gate: **166/166 original named function records**,
  **11,314/11,314 instruction bytes**, **417 supported relocations**
  in the existing accepted manifests.
- Strict named initialized-data gate: **244/244 objects**,
  **71,536/71,536 bytes**.
- Original `.bss` source-declaration gate: **12/12 symbols**, 18,437
  allocated bytes; **zero file-backed BSS bytes**.
- Original function queue: **166 EXACT**, **9 EXPERIMENTAL**,
  **123 UNRESOLVED**, 298 named function records in total.
- **New function promotions in this rebase/review: zero.**
- **Linked executable equality: not established.** The
  `whole_elf_gate.py` remains mandatory for the complete 314,338-byte
  canonical executable; no reconstructed linked candidate has passed.

The rebase and documentation review do not authorize wholesale
imports from the VCL `src/`, compiler flags, link scripts or
matching scorer into VUC.

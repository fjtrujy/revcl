# VUC 2004.1 — ASTNodeList Add ownership and near-exact printer probes

Date: 2026-10-14. Branch: `vuc`. Original comparison authority is the
unchanged canonical VUC 2004.1 Linux x86 ELF in the tracked upstream ZIP.
The historical compiler controls use GCC 3.3.3 pre-releases Jan 25 2004
and Dec 6 2003 with recovered GNU assembler 2.15.

## Repository and upstream

Checked active branch and Git status before editing, then executed
`git fetch origin && git rebase origin/main` successfully. No new
`origin/main` commits were available since the preceding batch, so
the rebase was a no-op. All **163 existing unrelated untracked paths**
remain untouched. No root VCL files, binary archives, build products, or
existing accepted source are modified by these probes.

The VCL `optDumb2.cpp` 2026-10-14 notes were reviewed:
`config/matching-evidence/optdumb2-split.md`. Its authentic target-only
separate compilation changes an original GCC **3.2.2** peephole2 and
register-renaming state, restoring two exact VCL functions. The result
does not prove original source-unit splitting in VUC, which instead uses
historical GCC **3.3.3**; no speculative source-unit split is promoted.

## 1. Recovering the real ASTNodeList<IR::Fragment_Data *>::Add

Target:
`TranslateFragList<MLGameString>(ASTNodeList<MLGameString>&)`,
original `0x0806e5b0`, **306 original bytes**. Its existing
strict original-length partial is **75/306 original-offset equal bytes**,
with **6/6 authenticated relocations**, a **10-byte contiguous exact
entry prefix**, and status **EXPERIMENTAL**, not accepted.

The observed original contains all three ASTNodeList fields,
16-element first allocation / capacity doubling, an 8-byte-alignment
optimized `cld; rep movsl` copy, and appends the newly constructed
12-byte polymorphic IR fragment. The existing provisional list member
is an ordinary inlined C++03 `Add(T)` definition in
`vuc/probes/translate_frag_list_string_partial_oct14.cpp`.

`vuc/probes/evaluate_fraglist_add_owner_oct14.py` compiled **175**
bounded source+GCC recipes. These vary real old-count lifetimes,
signed/unsigned capacities, conditional initialization, original/new
array pointer owners, `memcpy`/`memmove` and count expressions,
length update order, and branch-alignment/optimizer passes. The code
is extracted from the retained valid partial; no machine-code bytes,
padding, fabricated relocations, or forced register assignments occur.

- **53 emitted original-length 306-byte bodies**, but the highest
  original-length match stays **75/306**, **6/6 relocations** with the
  selected source's same 10-byte prefix.
- A natural `++length` spelling produces **311 emitted bytes** with
  **85 identical overlapping original-offset bytes**, but fails the
  mandatory 306-byte function length. It is *not* an independently
  accepted or selected partial.
- Deferring the pointer-store until after copying and explicitly
  retaining old capacity/length affect the useful compiler liveness;
  none yield a better strict-size reconstruction.

`vuc/probes/evaluate_fraglist_postlength_composition_oct14.py`
then tested **108** combinations of that actual `++length`
member spelling with the fragment's old/new input reference lifetime,
outer-loop increment point, copy operation and jump/code placement.
The best strict-length **306-byte** result is only **49/306**,
while the 311-byte `++length` form retains up to **85 overlapping
bytes** and the 10-byte common prefix. This rules out a straightforward
source compensation for the extra emitted instructions. The selected
**75/306** source therefore remains the best and is not changed.

The full per-recipe assembly/source/JSON findings are reproducible at
`/tmp/vuc-binutils215/fraglist-add-owner-next-oct14` and
`/tmp/vuc-binutils215/fraglist-postlength-composition-oct14`
inside the original Lima guest. No generated objects are committed.

## 2. Short printer: register allocator/source-owner diagnosis

The original `Print(ASTFunCall*, std::ostream&)` is **98 bytes**
at `0x0805b770`. The already selected experimental source, compiled
with original GCC 3.3.3 and `-O2`, reconstructs **96/98** with
authentic original call/literal relocations. Only offsets +23/+29
differ: GCC loads its callee field into **EAX**, whereas the original
loads it into **ECX**. The rest of the 98-byte instruction stream is
byte-identical after relocation/literal authentication.

The new `evaluate_printer_register_ownership_oct14.py` takes the
genuine `Printer.cpp` reconstruction used by already exact
`Printer_UglyPrint`, rather than fake compiler-state predecessors.
It tests natural C++03 const/local/reference/pointer callee owners
and the legitimate inlined member accessor with real GCC optimizer
pass controls and both original-era compiler snapshots.
Results are scored using
`vuc/config/print_ast_fun_context_oct14_probe.json`, which also
authenticates the four original printing literals. All binaries
remain in guest scratch under
`/tmp/vuc-binutils215/printer-register-source-owner-next-oct14`.

**Measured outcome:** **192** bounded compiler configurations (eight
actual callee source forms × twelve GCC pass/flag modes × both 2004-era
compiler snapshots). Exactly **160** retain the original 98-byte
function length; the other **32** produce wrong-sized 96-/91-byte
functions and are rejected by the strict matcher. Of original-sized
results, **80 score 96/98**, all retaining the identical two original
offset differences **+23** and **+29**. Another 80 original-sized
results match fewer bytes. None is **98/98**, so neither a pointer
alias, an inlined member getter, `-fno-regmove`, `-fno-gcse`,
`-fno-cse-follow-jumps`, `-fno-peephole2`, nor either recovered
GCC 3.3.3 snapshot changes the last pair of bytes while retaining
all original relocations and literals.

An independently compiled best-scoring `Printer.cpp` object under
plain `-O2` does **not** preserve the previously accepted neighboring
functions from that source unit: its `Printer_UglyPrint` body scores
**33/40** with two authentic relocations and its iostream global
startup wrapper scores **27/31** (the wrapper matches the compiler's
zero real relocations in this object). Those are deliberately
**negative source-unit preservation controls**, not changes to their
accepted objects or manifests. Retain the independent accepted
source-unit configuration until the actual emission/optimizer scope
is established.

## Remaining work and strict acceptance

For the list, do **not** add padding to hide source/growth differences.
Recover the genuine list member's template definition and original
translation-unit state, particularly the register and 0x1c-byte
stack-frame mapping. For the printer, a true earlier member
definition/optimizing source-owner context is preferable to
artificial register pinning.

All retained experiments are diagnostics unless function length,
every original relocation, every declared original literal, and the
full instruction stream match. The linked whole-ELF reconstruction
percentage remains unknown: an accepted-function gate alone cannot
establish it.

## Reproduction

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_fraglist_add_owner_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_fraglist_postlength_composition_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_printer_register_ownership_oct14.py"
python3 -m unittest discover -s vuc/tests -q
limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" \
    --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
    --out /tmp/vuc-binutils215/oct14-anotherbatch-baseline
limactl shell revcl-oracle -- python3 "$R/scripts/match_data_candidates.py" \
    --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
    --out /tmp/vuc-binutils215/oct14-add-owner-data
limactl shell revcl-oracle -- python3 "$R/scripts/match_bss_candidates.py" \
    --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
    --out /tmp/vuc-binutils215/oct14-add-owner-bss
```

The established before/after gate is **199 EXACT** of 298,
**14,621/14,621** accepted instruction bytes, **244/244** named data
objects (71,536 bytes), and **12/12** BSS symbols (18,437 bytes in
source allocations). The 22 experimental and 77 unresolved function
records remain unchanged unless a new fully authenticated match
is found.

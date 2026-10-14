# VUC: authentic compiler-snapshot and near-exact negative batch (2026-10-14)

Branch: `vuc`. Baseline was `e19c106` on top of VCL main `1693559`.
The repository's original binaries, VCL work, tracked acceptance source and
163 pre-existing untracked paths were preserved. There was no rebase in
this particular continuation batch.

## Outcome

**No new byte-exact function and no improved exact-length partial.**
The strongest independently recompiled near-exact candidates remain:

| Original symbol | Address | Original size | Best strict source result | Remaining difference |
| --- | --- | ---: | ---: | --- |
| `IsAccumulatorable(IR::Operator_Data*)` | `0x0805a570` | 77 | **74/77** | last case-11 branch direction and displacements: +73, +74, +76 |
| `Instruction` arena `__tcf_1` | `0x08051980` | 46 | **45/46** | dead scratch pop destination: +42 |
| `Print(ASTFunCall*, ostream&)` | `0x0805b770` | 98 | **96/98** | temporary load/argument register: +23, +29 |
| `MLTempString::MLTempString()` C2 | `0x08071c20` | 101 | **95/101** | compiler register allocations: +22, +28, +55, +60, +66, +70 |
| `MLTempString::Set(char const*, ...)` | `0x08071d00` | 117 | **111/117** | argument/initialization register allocations: +32, +42, +71, +76, +82, +86 |

Every result uses an actual emitted i386 ELF32 object, the canonical
original VUC symbol and the existing relocation-aware matcher. The
`IsAccumulatorable` fragment has zero relocations; all other named original
BSS/call/literal relocation sites are enforced in their corresponding
manifests. A matched length or a disassembly resemblance does not count
as exact acceptance.

## Two independently recovered historical GCC snapshots

The existing Linux VM lost its temporary toolchain on an earlier guest
restart. The January 25, 2004 GCC 3.3.3 prerelease, GNU as 2.15 and
header compatibility environment had already been restored in
`/home/fjtrujy.guest/.cache/vuc-toolchain`.

This round recovered the December 6, 2003 GCC 3.3.3 prerelease
independently from the repository's pinned
`vuc/scripts/bootstrap_dec06_gcc.sh`. Its source tarball and Bison package
were checksum-verified by that script, and its `cc1` and `cc1plus` are
built in the same VM. Its wrapper is installed persistently in
`/home/fjtrujy.guest/.cache/vuc-toolchain/binutils215/g++-dec06-as215`
with the historical GNU as 2.15 tool directory. The original VM path
`/tmp/vuc-binutils215` is a symlink to that persistent directory.
Neither GCC snapshot is claimed to be the original Debian-patched
`3.3.3 0pre3` compiler.

A fresh December compile of the accepted experimental accumulator source
still reports **77-byte size, 74/77 bytes matched, offsets
[73, 74, 76]**: the register and branch mismatch is not specific to the
January source snapshot.

## Four reproducible bounded source/flag investigations

1. `vuc/probes/evaluate_accumulator_inline_predicate_oct14.py`:
   Nine real, called, inlined C++ helper forms for the case-11 operand
   predicate (const pointer reference, nested conditional, explicit bool,
   GCC `always_inline` and template) with eight source-level call/guard
   forms and three historical flag modes. **216 recipes**. None changes
   the final orientation while retaining the authentic preceding
   74-byte prefix; the best remains 74/77.
2. `vuc/probes/evaluate_accumulator_nested_predictions_oct14.py`:
   Nineteen semantically equivalent boolean forms, nested
   `__builtin_expect`, explicit logical negation and ordinary C++ boolean
   coercion under eleven original-era branch, loop, label and block
   alignment/scheduling choices on *both* compiler snapshots.
   **418 attempted recipes**, none exact. Nested hints that do not
   correspond to an independently recovered original source form
   are negative compiler evidence only.
3. `vuc/probes/evaluate_vuc_near_exact_weak_controls_oct14.py`:
   Twelve old-GCC strong/weak binding, GCSE, block reorder, register
   move, basic-block probability, function-section and loop alignment
   variants across the five near-exact targets and two compiler
   snapshots. **120 attempted recipes**, zero exact. Source-unit
   `-fno-weak` can solve a real VCL GCC 3.2.2 deferred-definition
   owner, but does not transfer to these GCC 3.3.3 VUC sources.
4. `vuc/probes/evaluate_mltemp_constructor_prefix_oct14.py`:
   Nine actual source orders/subsets of the formatted constructor,
   default constructor and `MLTempString::Set`; ten historical C++
   optimizer modes; two recovered snapshots. **180 genuine-owner
   recipes**, all still at best 111/117 for `Set`. Unused helper
   declaration variants generated during the exploratory scratch
   search were discarded and are not part of the retained driver.
   No dummy helper is needed to reproduce the experiment.

Total retained matrix is **934 historical compiler recipes** (some
produce wrong-sized functions or cannot be scored because their source
unit's literal ownership is not authenticated). No fake executable
padding, forced register inline assembly, synthetic references or
manual relocation patching was introduced or accepted.

### Exact regression and remaining work

The canonical accepted manifest and source remain unchanged at
**197 EXACT / 14 EXPERIMENTAL / 87 UNRESOLVED** of 298 named symbols.
This batch does not change the selected 74/77 `IsAccumulatorable`
manifest and does not claim a full linked-ELF byte match.

A useful future step is source-level recovery of the original
`IsAccumulatorable` case-11 false-edge topology from its actual IR2Asm
translation-unit state, or evidence from the original author's C++
source/real compiler patches; further nested expect or artificial
helper syntax is not justified by these controls.

### Reproduce

Inside `revcl-oracle`, with GCC 3.3.3 and GNU as 2.15 restored:

```sh
R=/path/to/revcl/vuc
python3 "$R/probes/evaluate_accumulator_inline_predicate_oct14.py"
python3 "$R/probes/evaluate_accumulator_nested_predictions_oct14.py"
python3 "$R/probes/evaluate_vuc_near_exact_weak_controls_oct14.py"
VUC_SNAPSHOT=dec06 python3 "$R/probes/evaluate_vuc_near_exact_weak_controls_oct14.py"
python3 "$R/probes/evaluate_mltemp_constructor_prefix_oct14.py"
VUC_SNAPSHOT=dec06 python3 "$R/probes/evaluate_mltemp_constructor_prefix_oct14.py"
```

All generated sources and compiler objects live under
`/tmp/vuc-binutils215`, and the historical compilers in the persistent VM
cache. The probe flags/sources in `vuc/config/exact_candidates.json`
were not changed.

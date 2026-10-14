# VUC: latest VCL compiler-state lessons and genuine-source negative batch

Date: 2026-10-14. Branch: `vuc`.

## Rebase and preservation

Fetched `origin/main` at **`25dc19f`** and rebased the VUC branch
from `db86285` onto that commit, replaying **116 commits** without
conflicts. The resulting initial HEAD was `1ef0dd3`. There were no
uncommitted tracked changes before the rebase, and the **163 existing
untracked paths** were preserved. The unrelated experiments in those
paths and the archived upstream VUC/VCL binaries remain untouched.

## What the latest VCL main branch teaches

The newest VCL work is *not* a matching VUC source/compiler: its original
compiler is **GCC 3.2.2**, whereas the VUC ELF identifies **GCC 3.3.3
prerelease**. Nonetheless, the following real compiler phenomena are
worth checking against VUC:

- VCL `earliestInsert` moved from **921/937** to **937/937**, and
  `latestInsert` from **275/279** to **279/279**, when *unchanged
  genuine functions* were moved into their own source unit. The VCL
  evidence traces this to GCC's compilation-process-lifetime
  `peep2_find_free_register` scratch-search state, which also affects
  later live register renaming. The split is an authorized
  reconstruction; it does not establish the historical Makefile.
- VCL's `RegWeb` functions respond to a genuine strong owner and
  `-fno-weak` (rather than simple `-ffunction-sections`), affecting
  deferred function binding, actual source-section alignment and calls.
- A new VCL negative note says `-fno-regmove` can alter the RTL
  `.18.regmove`/pseudo structure while leaving final physical
  registers and machine bytes unchanged. A pass dump difference is not
  itself evidence of byte identity.

All transfer experiments here use actual GCC **3.3.3 (2004-01-25
prerelease)** and the verified GNU as **2.15** inside the existing Lima
guest, with canonical VUC ELF symbols and the strict relocation matcher.
No copied instruction bytes, manufactured padding, dummy compiler-state
functions, artificial register constraints or fabricated address
mappings were used.

## Original-order Instruction source unit: still 45/46

**Target:** original `Instruction` static cleanup `__tcf_1`,
`0x08051980`, **46 bytes**.

Driver: `vuc/probes/evaluate_instruction_true_source_order_oct14.py`.

This creates ordinary C++03 fragments from the already reconstructed
`InstructionList` and `Instruction` methods. In the main form, methods
are compiled in *original ELF symbol order*: `Add(Instruction*)`,
`Add(const char*,...)`, `AddB`, `AddL`, `AddLineComment`,
`GenerateImage`, `GenLabel`, `GenTmp`, `Print`,
`Instruction::ToString`, `operator new`, followed by the genuine
static arena allocator/cleanup. `GenerateImage(unsigned char*)` is an
empty real member function whose original 5-byte body was independently
inspected; it is not a dummy to seed a register search.

Seven genuine definition orders, three static declaration placements,
with/without the real `GenerateImage` member, and seven period-GCC
compilation options yield **294 historical source configurations**.
`<iostream>` is retained so the original-style `__tcf_0` appears
before `__tcf_1`. A preliminary version omitted that header, emitted
`__tcf_0` as the target, and was discarded before scoring.

**Outcome:** The strict checker still reaches **45/46** at all best
original-length candidates; the sole mismatch remains offset **+42**,
a dead cleanup-pop scratch destination (**EBX** instead of original
**ECX**). All three original relocation sites in those candidates are
authenticated. The candidate machine code and the original byte match
do **not** justify byte-exact promotion.

Genuine reorderings, `-fno-regmove`, `-mbranch-cost=0`, jump
alignment, and the source-unit reset do not recover the original
scratch decision. A more accurate original owner/predecessor
reconstruction or authenticated original compiler patch level would be
needed; this experiment alone does not reveal which.

## MLTempString::Set: genuine named-pointer lifetimes still 111/117

**Target:** `MLTempString::Set(const char*, ...)`, `0x08071d00`,
**117 bytes**.

Driver: `vuc/probes/evaluate_mltemp_set_pointer_alias_oct14.py`.

Because named `const char *const &` source lifetimes previously
unlocked a separate accepted `MLGameStringTable::Add`, this batch
checks 19 genuine source patterns around `format`, `va_list`,
`MLTempStringData::Get()`, `this`, and the returned formatted pointer
under seven GCC options (including `-fno-regmove` and
`-mbranch-cost=0`): **133 configurations**.

**Outcome:** The best remains **111/117**, with the same six
register-choice differences at relative offsets **32, 42, 71, 76,
82, 86**. All actual global-storage and named-call relocations remain
checked by the original `ml_temp_set_probe.json`. This disproves the
simple transfer of the other function's pointer-reference trick; it
does not disprove the original C++ semantics.

## VCL strong-owner flag does not rescue VUC STL/symbol-table helpers

Driver: `vuc/probes/evaluate_vcl_weak_stl_transfer_oct14.py`.

Twelve bounded GCC option configurations, prominently
`-fno-weak` and combinations with original-era jump/loop/label
alignment, were applied to *actual historical libstdc++ instantiations*
and `SymbolTable` source, without writing substitute implementations.

| VUC function | Original length | Outcome |
| --- | ---: | --- |
| `std::__uninitialized_copy_aux<Scope*,Scope*>` | 120 | No authenticated original-length body; typical candidate **115/127** |
| `std::vector<Scope>::reserve(unsigned)` | 241 | No authenticated original-length body; typical candidate **237/257** |
| `SymbolTable::Enter` (real source unit) | 185 | No original-length candidate; common output **180/182/194** |
| `SymbolTable::Enter` (isolated real method) | 185 | Same failure; changing the GCC weak-owner option is insufficient |

The 513-bucket scope layout and authentic `SymbolTable` methods
remain source-derived rather than manually crafted machine bytes.
`-fno-weak` here does not establish a hidden historical VUC
compilation policy. This is **48 compiler controls** across four
source owners, no newly accepted exact function.

## Toolchain resilience discovered during the full acceptance gate

The existing Lima VM had earlier been restarted and cleared `/tmp`,
which prompted the persistent, checksum-verified compiler cache work.
In this batch, the full exact-function regression exposed another
bootstrap requirement: the accepted C-language `LibcCsu.c` needs
historical GCC's **`cc1`**, in addition to the C++ `cc1plus`
frontend. The pinned January and December bootstrap scripts now
build both frontends; the cache restoration script checks that both
exist before declaring a usable toolchain. The January 2004 `cc1`
was compiled successfully without modifying source archives or VCL.

The existing byte-exact candidate ledger and canonical binary were
not edited. No function was promoted merely on a byte count or size
coincidence, and **whole ELF byte equality remains unachieved**.

The complete independent regression suite passed after rebuilding `cc1`:

- **197/197** exact accepted functions, **14,417/14,417** original
  instruction bytes;
- **244/244** authenticated initialized-data objects,
  **71,536/71,536** original initialized-data bytes;
- **12/12** BSS declarations, **18,437** source-allocation bytes
  (these represent no file-backed original data);
- **149/149** host tests pass; the intentionally failing negative
  byte-match fixture is an expected passing test.

The authoritative original-symbol ledger remains **197 EXACT / 14
EXPERIMENTAL / 87 UNRESOLVED**. No new byte-exact acceptance was made.

## Reproduction

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_instruction_true_source_order_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_mltemp_set_pointer_alias_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_vcl_weak_stl_transfer_oct14.py"
```

Run after restoring cached compiler aliases using
`vuc/scripts/restore_cached_toolchain.sh`. Objects, generated test
sources, compare data and GCC dumps remain outside Git under
`/tmp/vuc-binutils215` in the VM.

**Next substantive work:** recover authentic original source-unit
predecessor boundaries for the `Instruction` destructor rather than
loop syntactic permutations; use GCC `flow2` → `peephole2` →
`rnreg` evidence for the particular EBX/ECX scratch decision.
For `MLTempString::Set`, recover the actual default-constructor
control-flow/field initialization order before revisiting register
flags. Revisit the small AST statement-list translator only with
genuine neighboring-function or original IR class-layout evidence.

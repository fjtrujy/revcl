# VUC: unresolved fragment-list and symbol-scope batch

Date: 2026-10-14. Working branch: `vuc-unresolved-oct14`, created directly
from `vuc` at `39dfe83`. This is a **separate Git branch, not a worktree**.
The original VUC ELF/ZIP and the VCL root reconstruction remain unchanged.

## Purpose and outcome

Set aside the existing near-exact `Value`, Flex, printer, and Instruction
targets. Investigate other genuinely **UNRESOLVED** functions, prioritizing
source-level explanations of GCC 3.3.3 code generation.

**No newly byte-exact function was recovered.** The batch does add a
reproducible, genuine C++03 implementation skeleton for two original
`TranslateFragList` template instantiations and documents its exact
failure boundary. Neither translation is promoted to accepted source.

| Original function | Address | Length | Best original-length result | Status |
| --- | --- | ---: | ---: | --- |
| `TranslateFragList<ASTVarDecl*>` | `0x0806e0a0` | 240 | **41/240**, two checked relocations | Unresolved; source probe |
| `TranslateFragList<ASTDecl*>` | `0x0806e190` | 240 | **41/240**, two checked relocations | Unresolved; source probe |
| `MLHashTable<...513>::MLHashTable(const&)` | `0x0804f420` | 166 | No original-length candidate | Unresolved |
| `std::__uninitialized_copy_aux<Scope*,Scope*>` | `0x0804f220` | 120 | No original-length candidate | Unresolved |

The two 41-byte measurements are **not** new accepted instruction bytes and
are not counted toward the exact coverage. They are only exact bytes in a
partial, independently relocation-checked experimental object.

## 1. Fragment-list source reconstruction

Source: `vuc/probes/translate_frag_list_oct14.cpp`.
Relocation-authenticated original-length manifests:
`vuc/config/translate_frag_list_{var,decl}_oct14_probe.json`.

The original two 240-byte bodies provide unusually direct evidence about the
data structure and operation sequence:

- The returned `ASTNodeList<IR::Fragment_Data*>` is constructed by hidden
  struct-return pointer and has three **32-bit** fields: count at +0, capacity
  at +4 and entry-array address at +8. Its constructor writes all three zeros.
- Both functions read the same source list layout, iterate by signed index,
  load one pointer from `entries[i]`, call the **same authentic**
  `TranslateFrag(ASTDecl*)` at `0x0806d2c0`, then append the result.
- Appending grows a full list to **16 elements** when empty or doubles
  nonzero capacity. The compiler calls `operator new[](unsigned)` at
  `0x080499ac`, copying 4-byte pointers with an inline
  `rep movsl` sequence and a leading 4-byte alignment optimization.
- The original path does not call `delete[]` on old storage in these
  functions; the provisional source mirrors the visible call graph but
  does **not** assert complete application ownership semantics.
- `ASTVarDecl*` is passed through the recovered `ASTDecl*` inheritance
  relationship, so the two template specializations have the same API-level
  behavior and original body structure.

The first natural `-O2 -frename-registers -falign-jumps=1` instantiation
compiled to **234 bytes**, while the second instantiation compiled in the
*same translation unit* shrank to **86 bytes**, with a separate call to
`ASTNodeList<Fragment*>::Add`. This is a real historical GCC inlining /
emission-context effect: compiling each specialization in a standalone
object produces the larger inline variant, rather than assuming source
symbol order or source-unit compilation context is irrelevant.

For the isolated 234-byte source, `-fno-reorder-blocks` yields **241 bytes**
and places the loop header at relative **+0x40**, matching the original
loop's location and the original forward exit structure. Its initial
instruction sequence through the first `TranslateFrag` call substantially
agrees with the original structure, but it is **not** an original-length
candidate and therefore has **no accepted 240-byte match score**. Differences
in stack homes for the list index, translated pointer, saved old count and
new allocation result remain.

The 240-byte source variants from the reproducible bounded evaluator each
match **41/240** after strict original-target relocation resolution. Their
high mismatch count and different initial stack/loop topology rule them out
as exact reconstructions. Merely matching the target length by changing
loop shape is insufficient.

### Evaluated controls

- `evaluate_translate_frag_list_oct14.py`: seven natural append/ownership
  implementations, three outer-loop forms, two genuine target
  instantiations and five GCC 3.3 flag contexts (210 compiler builds).
  Tested source pointer indexing, saved old buffer, field assignment before
  or after copy, capacity/store lifetime, `memcpy` and direct pointer-copy
  loops. The 240-byte best remained 41/240 for both instantiations.
- `evaluate_translate_frag_list_layout_oct14.py`: three ordinary outer
  source-loop forms across historical loop/jump/label controls with
  `-fno-reorder-blocks` (192 builds). The selected 241-byte true
  forward-loop graph was not reduced to the target 240 bytes; none gave an
  original-length better match.
- `evaluate_translate_frag_list_lifetimes_oct14.py`: six authentic
  count/pointer lifetimes, two instantiations and four alignment/reordering
  contexts (48 builds). Saved count variables alter emitted length and stack
  homes (229 to 242 bytes in the tested variants), but none was an exact
  240-byte object.

These findings support revisiting true GCC 3.3 translation-unit predecessor
processing and actual `ASTNodeList::Add()` definition/inlining boundaries,
not artificial NOPs or register constraints. The original two 240-byte
functions might share a method definition processed earlier in the original
`Translate.cpp` unit; the independent C++03 source is a hypothesis,
not proof of original lexical authoring.

## 2. MLHashTable 513-bucket copy constructor

Source: `vuc/probes/hash_copy_constructor.cpp`.
Evaluator: `vuc/probes/evaluate_hash_copy_constructor_oct14.py`.

The authentic original constructor is **166 bytes** and invokes
`operator new(unsigned)`, `MLGameString` copy construction, the
delete cleanup and `_Unwind_Resume`, as previously recorded. The natural
source uses a 513-bucket loop, linked-node copy construction and an insertion
tail pointer.

The original's bucket index resides in a stack slot and its inner loop
saves pointers around `new` / `MLGameString` construction. The current
C++ source with historical GCC emits **162 bytes**, with a different live
register/index recurrence. Bounded ordinary variants test int/unsigned/short
indexes, `for`/`while` bucket loops, tail address spelling, separate
node traversal and link assignment order. Historical loop/jump alignment
and register-renaming controls do not recover the original 166-byte
body. Do not claim that padding alone explains this residual: useful
register/induction topology differs much earlier.

## 3. STL uninitialized 513-bucket scope copying

Source: `vuc/probes/scope_uninitialized_copy.cpp`.
Evaluator: `vuc/probes/evaluate_scope_copy_historical_snapshots_oct14.py`.

This is a real historical `std::__uninitialized_copy_aux<Scope*,Scope*>`
template instantiation; no fake library function is substituted.
Unlike earlier Jan25-only alignments, this batch also tested the
**2003-12-06 GCC 3.3.3 prerelease** with the same GNU as 2.15 wrapper.
The 96 builds cross two historical snapshots, both register-renaming
settings, four jump alignments, three loop alignments and
`-fuse-cxa-atexit` on/off.

Neither snapshot produces an original-sized 120-byte body in these
controls; the tested outputs include 115, 119 and 127 bytes. The
existing original ELF shows the cleanup iterator advanced before
the `MLHashTable::clear()` call, while the standard-library candidate's
exception cleanup differs. This must be investigated through actual
STL template source/exception lowering, not by patching the bytes.

## Reproduce

From the existing Mac repository, without creating a worktree:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_translate_frag_list_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_translate_frag_list_layout_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_translate_frag_list_lifetimes_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_hash_copy_constructor_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_scope_copy_historical_snapshots_oct14.py"
```

All generated sources and .o/RTL artifacts stay under
`/tmp/vuc-binutils215` **inside the VM**. Every full-score measurement
uses `vuc/scripts/match_probe.py` with the original ZIP-backed ELF,
historical function symbol and all supported relocation targets.

## Handoff

Leave the near-exact Value/Flex/Instruction batch alone until a separate
visit. Continue other unresolved targets by choosing original source
structures with no existing probe, then recover correct lexical
types/exception lifetime *before* optimizing block alignment. The most
promising direct continuation of this batch is authentic
`ASTNodeList::Add()` source and the original `Translate.cpp`
source-unit processing order.

The accepted-function manifest and `vuc/src/` remain unchanged.
The authoritative inventory therefore remains **195 EXACT, 15
EXPERIMENTAL and 88 UNRESOLVED** of 298 named functions. Whole-ELF
identity is not established.

# VUC historical optimizer and source-ownership controls

Date: 2026-10-16. Target: the unchanged archived VUC 2004.1 Linux i386
ELF32. Branch: `vuc`. Compiler: recovered GCC 3.3.3 prerelease
(2004-01-25) and GNU as 2.15. All tested sources are ordinary C++03;
no inline assembly, executable byte patches, fabricated relocations,
or artificial padding were used.

## Value::operator+ x87 scalar/vector tail

The selected original-length candidate at `0x0806e9b0` remains
**203/219** matching original instruction bytes, with its authentic
`TypeConvert` relocation. The first 169 bytes still match consecutively.
The original code emits four ascending vector lanes and a separate scalar
float case. Historical GCC canonicalizes source `switch` case order and
shares a tail between the scalar case and a vector lane.

`vuc/probes/evaluate_value_plus_crossjump_oct16.py` tests both the
selected lexical lane order (3,1,2,0) and ascending order (0,1,2,3)
against four historical optimizer controls:

| Source | GCC control | Compiled size | Strict original-offset result |
| --- | --- | ---: | --- |
| Selected lane order | `-fno-crossjumping` | 216 | Wrong length |
| Selected lane order | `-fno-gcse` | 219 | 203/219, unchanged |
| Selected lane order | `-fno-reorder-blocks` | 202 | Wrong length |
| Selected lane order | `-fno-cse-follow-jumps` | 219 | 203/219, unchanged |
| Ascending lane order | `-fno-crossjumping` | 216 | Wrong length |
| Ascending lane order | `-fno-gcse` | 216 | Wrong length |
| Ascending lane order | `-fno-reorder-blocks` | 218 | Wrong length |
| Ascending lane order | `-fno-cse-follow-jumps` | 216 | Wrong length |

Disabling cross-jumping does **not** recover the desired 219-byte
case separation. Only the 219-byte objects can be compared as eligible
partials; both stay 203/219, with the original mismatch offsets
`+169,+172,+175,+194..+206`. Do not repeat case permutations or
these optimizer switches without new source-unit or RTL evidence.

Reproduce in `revcl-oracle`:

```sh
python3 /path/to/revcl/vuc/probes/evaluate_value_plus_crossjump_oct16.py
```

## ParseArguments global-array alignment

The original `ParseArguments(int,char**)` at `0x0805b3e0` is
**777 bytes**. The existing behavioral source recovers filename-length
checks and output-branch relations but remains wrong-length.

`vuc/probes/evaluate_parseargs_aligned_copy_oct16.py` tests a
plausible source-level reason that `__builtin_memcpy` might avoid the
extra alignment loop in historical GCC: declarations of the original
256-byte global filename/path arrays with actual `aligned` attributes.
It uses `-O2 -frename-registers -minline-all-stringops -fno-builtin
-falign-jumps=1 -falign-labels=1 -falign-loops=4`, with an explicit
`__builtin_memcpy` for the variable-length input-directory prefix.

| Declared buffer alignment | Candidate size |
| --- | ---: |
| No additional alignment | 772 |
| Input only, 4 bytes | 772 |
| Destination only, 4 bytes | 772 |
| Both, 4 bytes | 723 |
| Both, 8 bytes | 676 |
| Both, 16 bytes | 676 |

The compiler is sensitive to *joint* source/destination alignment,
not one buffer alone, and its lowering changes by tens of bytes.
None equals the original 777 bytes, so none was given a strict score
or added to the experimental manifest. The binary addresses being
4-byte aligned alone do not authenticate original source declarations.

Reproduce:

```sh
python3 /path/to/revcl/vuc/probes/evaluate_parseargs_aligned_copy_oct16.py
```

## Binder_Bind label alignment and main-name lifetime

The original `Binder_Bind(ASTModule*)` at `0x0804e980` is
**552 bytes**. The source recovers the stack-owned `SymbolTable`,
scope-vector reserve/cleanup, three AST module list binds and local
`main` lookup, but is not original-length.

`vuc/probes/evaluate_binder_original_alignment_oct16.py` checks
nine actual GCC-generated alignment combinations. With
`-O2 -frename-registers`, the results are:

| Jump / label / loop alignment | Candidate size |
| --- | ---: |
| 8 / 1 / 1 | 525 |
| 8 / 2 / 1 | 533 |
| 8 / 4 / 1 | 549 |
| 8 / 1 / 2 | 525 |
| 8 / 1 / 4 | 533 |
| 8 / 1 / 8 | 541 |
| 4 / 8 / 1 | 573 |
| 4 / 8 / 4 | 573 |
| 4 / 8 / 8 | 573 |

The newly closest **549-byte** object is still three bytes too short
and cannot be called a strict 549/552 partial. To test whether a
real local lifetime might affect a remaining branch boundary,
`vuc/probes/evaluate_binder_mainname_alignment_oct16.py` builds
twelve C++03 name-lookup variants under
`-falign-jumps=8 -falign-labels=4 -falign-loops=1`.
Eleven emit 549 bytes and one explicit local-entry pointer emits
557. No 552-byte candidate appeared. These are negative controls,
not compiler flags accepted for production.

Reproduce:

```sh
python3 /path/to/revcl/vuc/probes/evaluate_binder_original_alignment_oct16.py
python3 /path/to/revcl/vuc/probes/evaluate_binder_mainname_alignment_oct16.py
```

## Acceptance boundary

No candidate from this batch met a **new** original-length,
relocation/literal-authenticated higher score or complete byte-exact
body. The previously selected 203/219 Value partial remains unchanged;
`Binder_Bind` and `ParseArguments` remain unresolved. Neither the
accepted `vuc/src/` functions nor either matching manifest was changed.

The useful result is a reproducible restriction on four concrete
compiler/source hypotheses, preserving the distinction between
wrong-length overlap and real matching original-offset bytes. Future
work should use new ELF/DWARF and real translation-unit evidence
rather than another equivalent spelling or alignment lottery.

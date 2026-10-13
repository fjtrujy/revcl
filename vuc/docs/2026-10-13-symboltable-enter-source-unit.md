# Reconstructing VUC SymbolTable startup from genuine hash-table methods

Date: 2026-10-13. Branch: `vuc`.

## Newly accepted original function

`_GLOBAL__I__ZN11SymbolTable5EnterERK12MLGameStringP16SymbolTableEntry`
at original `0x0806a530` now matches **31/31 instruction bytes** under
the recovered January 2004 GCC 3.3.3 compiler with GNU as 2.15. Its
sole relocation resolves to the original `__static_initialization_and_destruction_0(int,int)`
at `0x0806a2e0`. The source is ordinary C++ in
`vuc/src/SymbolTableUnit.cpp`, built with
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`.
No dummy functions, original byte copying, injected assembly, or manual
register patching was needed.

## Source-unit evidence

The original `SymbolTable.cpp` order is `Enter` at `0x0806a330`, `Find`
at `0x0806a3f0`, `BeginScope` at `0x0806a470`, `EndScope` at
`0x0806a500`, and the startup callback at `0x0806a530`.

The accepted source-unit fragment contains the real `Enter`, `Find`,
and `BeginScope` methods in that order. The `Enter` reconstruction is
**not accepted byte-exact**; it inserts a new key/hash/value/next node
into the final 513-bucket scope from a GCC 3.3 vector of scope tables.
However, this source context causes the compiler to generate the
original wrapper bytes. The same candidate object also preserves:

| Previously accepted function | Original address | Check |
| --- | --- | --- |
| `SymbolTable::Find` | `0x0806a3f0` | 115/115, one relocation |
| `SymbolTable::BeginScope` | `0x0806a470` | 140/140, five relocations |
| Global constructor wrapper | `0x0806a530` | **31/31, new** |

`vuc/probes/evaluate_symboltable_unit_oct13.py` shows that the source
unit containing `Enter` only, or `Enter` plus `Find`, is **29/31**.
Adding the already verified `BeginScope` after the other two authentic
methods yields **31/31**. This matches prior evidence that GCC's
source-unit code generation can affect caller register selection.

## Useful unaccepted `Enter` evidence

The original `SymbolTable::Enter()` is 185 bytes with calls to
`operator new(unsigned)`, `MLHashFun<MLGameString>::Hash`, the
`MLGameString` copy constructor, `operator delete(void*)`, and
`_Unwind_Resume`. `vuc/probes/symboltable_enter_oct13.cpp` reproduces
all five named calls and the real 16-byte hash-node layout.

The initial `.back()` vector form compiled to 162 bytes and omitted
original vector-size division by 2052. Replacing it with
`scopes_[scopes_.size() - 1]` reproduces the original constant-multiply
scope-index computation and the entire leading instruction block,
but the resulting function is **194 bytes**, not 185. With
`-falign-jumps=1` it becomes 180 bytes. Ordinary slot-reference,
slot-pointer, local-old-node, and index variants do not yet reproduce
the original 185-byte exception landing pad and insertion-store order.
The 96-form bounded experiment is available as
`vuc/probes/evaluate_symboltable_enter_oct13.py`.

Neither the 185-byte original function nor the incomplete translation
unit has been promoted by treating its near-match as exact. The original
library headers/patchset, all SymbolTable operations, and true linker
layout remain incomplete.

## Reproduce

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-symboltable-174
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_symboltable_unit_oct13.py
```

This milestone increases the accepted inventory to **174/298 exact
function records**, **11,739/11,739 instruction bytes**. After the
subsequent bounded partial-candidate batch, the ledger has nine
experimental and 114 unresolved original function records. It does not
establish whole-ELF byte-for-byte identity.

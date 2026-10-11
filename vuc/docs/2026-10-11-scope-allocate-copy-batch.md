# VUC byte-exact scope vector allocation/copy reconstruction

Date: 2026-10-11. Branch: `vuc`.

## Accepted original function

The historical GCC **3.3.3 20040125 prerelease** compiler with GNU **as 2.15** produces the exact **143/143 instruction bytes** for:

```text
std::vector<MLHashTable<MLGameString,SymbolTableEntry*,MLHashFun<MLGameString>,513>>
    ::_M_allocate_and_copy<Scope*>(unsigned, Scope*, Scope*)
Original address: 0x0804f130
```

The source fragment `vuc/src/ScopeAllocateAndCopy.cpp` is an explicit
libstdc++ 3.3 vector member instantiation, not an assembly transcription.
Its `-O2 -frename-registers -falign-jumps=1 -falign-labels=2`
recipe reproduces the original symbol's code and **all seven original
relocations**, including the specialized `__uninitialized_copy_aux`
and std allocator operations, `__cxa_begin_catch`,
`__cxa_rethrow`, `__cxa_end_catch` and `_Unwind_Resume`.
The unwind cleanup flow is thus part of the exact acceptance check,
not a hypothetical normal-return-only implementation.

This is a particularly useful cross-function result: the original
`Scope` object is a 513-bucket (`0x804` byte) template and the
newly accepted routine depends on the original copy constructor/
uninitialized-copy ABI that remains under investigation.

## Strict scope of verification

The full named-function gate was rerun against the authentic ZIP-backed
VUC 2004.1 executable. It passed **148/148 accepted function records**,
**8,468/8,468 original instruction bytes**, **377 resolved relocations**
and **132 original literal bytes checked per manifest**. The original
function-symbol inventory currently has **148 EXACT**, **10 EXPERIMENTAL**
and **140 UNRESOLVED** out of 298 records.

This is **not** a claim that the original translation units, historical
Debian GCC patchset, linked executable, debug sections, ELF headers or
unreconstructed code and data are byte-identical. Complete executable
byte equality remains a separate mandatory acceptance requirement
enforced by `vuc/scripts/whole_elf_gate.py`.

## Reproduction

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-148-resume
```

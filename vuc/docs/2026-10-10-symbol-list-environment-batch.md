# Exact VUC application batch: scope lookup, listing, hash environments and assembly

Date: 2026-10-10. Branch: `vuc`.

## Mandatory byte-exact results

The original VUC 2004.1 ELF and date-matched GCC 3.3.3 (Jan 25 2004)
with GNU as 2.15 verify **106/106 named function-symbol records**,
**5,268/5,268 instruction bytes**, **218 resolved original ELF32
relocations**, and **35 verified original referenced literal bytes**.
There are **50 passing host tests**. This does **not** establish
byte-for-byte equality of the complete ELF, its layout or toolchain.

| Function | Original address | Exact bytes |
| --- | --- | --- |
| `SymbolTable::Find(MLGameString const&,bool)` | `0x0806a3f0` | 115/115 |
| `ListFile::Print(int,int,std::ofstream&)` | `0x0805ab60` | 88/88 |
| `ListFile::PrintLine(int,std::ofstream&)` | `0x0805aad0` | 133/133 |
| `__tcf_1` (`globalEnv` teardown) | `0x080502a0` | 16/16 |
| `__tcf_2` (`localEnv` teardown) | `0x080502b0` | 16/16 |
| `AssembleModule(IR::Module_Data*,InstructionList&)` | `0x08054140` | 89/89 |

Six new original records contribute **457** additional exact instruction
bytes compared with the prior 100-function checkpoint.

## SymbolTable::Find

The original `SymbolTable::Find()` iterates the historical
`std::vector<MLHashTable<MLGameString,SymbolTableEntry*,...,513>>`
from its most recent scope backward, invokes the existing exact
`MLHashTable::FindCopy()`, and stops at the first hit. If local-only
lookup is enabled it **breaks** from the loop after the first miss.
Expressing this as `break` rather than returning null inside the
loop reproduces the original GCC epilogue and control flow exactly.

The 513-bucket hash table has 2,052 bytes (`0x804`), which explains
the original scope traversal stride and multiply constant. The
recovered declarations are provisional, not authenticated source.

## ListFile::Print and PrintLine

`Print()` adjusts negative start positions, clamps the end position
to the original line count and prints each selected line via
`PrintLine()`. `PrintLine()` checks the one-based line index,
accesses a `std::vector<std::string>`, outputs the original
`"; "` literal followed by the string, and emits `std::endl`.
Its reference to the historical GNU `std::string::_Rep::_S_terminal`
and both original output operator targets are byte-verified.

Each function is reproduced with ordinary original-era C++. Three
specific compiler modes and source boundaries were tried:

- `ListFile::PrintLine()` is 133/133 with `-O2 -frename-registers`
  in a separate source fragment `vuc/src/ListFilePrintLine.cpp`.
- `ListFile::Print()` is 88/88 with
  `-O2 -frename-registers -falign-jumps=1` in `vuc/src/ListFile.cpp`.
- `GetNumLines()` stays 18/18. Combining all three in one candidate
  translation unit changed the compiler's register allocation for
  `PrintLine()`, so the two exact fragments remain separate until
  the original compilation boundaries can be authenticated.

## Global environment teardown

Original BSS symbols `globalEnv` at `0x0808ac40` and `localEnv`
at `0x0808b460` are each exactly `0x804` bytes and instantiated
as ordinary `MLHashTable<MLGameString,MLGameString,...,513>`
objects. The generic template's destructor invokes `clear()`.
Including `<iostream>` in the source fragment gives the original
compiler-generated `__tcf_0` callback first, followed by
`__tcf_1` for `globalEnv` and `__tcf_2` for `localEnv`.

Each original 16-byte callback is fully exact, and both its BSS
object symbol and linked `clear()` target are authenticated against
the original ELF. The original one-per-concept template is retained;
no manually encoded startup bytes were inserted. The prior exact
hash operations also remain passing.

## AssembleModule

The module has an initial status flag followed by an
`ASTNodeList<IR::Fragment_Data*>`. The routine returns immediately
when flagged and otherwise calls `Assemble()` once per fragment.
Declaring `IR::Fragment_Data **entries = fragments.entries`
inside the loop before passing `entries[i]` reproduces the
original GCC argument-evaluation order. This solved the final seven
nonmatching bytes (82/89 to 89/89) without assembly or padding.

## Remaining work and strict boundaries

`InstructionList::GenTmp()` has a promising 85/85 *isolated
candidate* when compiled together with a non-exact
`GenLabel()` hypothesis. Its standalone/combined accepted-source
form does not yet match, and the non-exact sibling cannot be
considered reconstructed. Both remain probes pending source-order
recovery. `Canonize2_Reduce()` is 83/85; the two differences
are compiler register selection at the initial status test.
`yy_create_buffer()` is 131/139. None has been admitted to
the exact manifest.

The canonical inventory is **106 EXACT, 8 EXPERIMENTAL and 184
UNRESOLVED** out of 298 identified original text-function records.
No such counts constitute a whole-binary match percentage.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-106
```

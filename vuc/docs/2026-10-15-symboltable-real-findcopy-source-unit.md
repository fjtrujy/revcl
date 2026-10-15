# VUC SymbolTable.cpp: genuinely co-instantiated FindCopy and source-unit lifetime

Date: 2026-10-15. Original VUC 2004.1 Linux i386, two independently
recovered GCC 3.3.3 prerelease snapshots and GNU binutils 2.15.

The original ELF symbol/source-file sequence shows `SymbolTable::Enter`
(`0x0806a330`), `SymbolTable::Find` (`0x0806a3f0`),
`SymbolTable::BeginScope` (`0x0806a470`),
`SymbolTable::EndScope` (`0x0806a500`), the weak
`std::vector<SymbolScope>::_M_insert_aux` (`0x0806a550`),
its `__uninitialized_copy_aux<normal_iterator<SymbolScope*>>`
(`0x0806a840`), and `MLHashTable<MLGameString,SymbolTableEntry*>::FindCopy`
(`0x0806a8e0`). These functions' genuine C++03 template ownership has
been incompletely reconstructed in previous batches.

## Recovery

`vuc/probes/evaluate_symboltable_findcopy_true_tu_oct15n.py` combines
only *genuine recovered code*: the original-order
`vuc/src/SymbolTableUnit.cpp` implementation and the source-authenticated
`MLHashTable::FindCopy` template method already reconstructed separately
in `vuc/src/MLHashTableFindCopy.cpp`. It tests the real FindCopy
specialization after `EndScope` (as the ELF weak-template placement
suggests), the full existing two-specialization source context, and a
deliberate placement before `BeginScope` as a negative ownership
control. Both historical compiler snapshots use the original
`-O2 -frename-registers -falign-jumps=1` recipe.

**All forms with an explicit template definition simultaneously yield**
the original ELF instruction bytes and authentic relocations for four
relevant accepted methods:

| Function | Original address | Verification |
| --- | --- | --- |
| `SymbolTable::Find` | `0x0806a3f0` | **115/115**, 1 relocation |
| `SymbolTable::BeginScope` | `0x0806a470` | **140/140**, 5 relocations |
| `SymbolTable::EndScope` | `0x0806a500` | **27/27**, 1 relocation |
| `MLHashTable<MLGameString,SymbolTableEntry*>::FindCopy` | `0x0806a8e0` | **114/114**, 2 relocations |

The latter was already independently accepted exact using the temporary
standalone `MLHashTableFindCopy.cpp`. We have now reinstated its ordinary
template method definition and exactly this specialization **after
EndScope** in the accepted `vuc/src/SymbolTableUnit.cpp`, and updated
`vuc/config/hash_symbol_findcopy_probe.json` to compile it from the real
SymbolTable source unit. This improves original translation-unit
provenance without claiming an additional exact function.

### Honest negative results

Across original-order placements and both snapshots, the nearby
`std::vector<SymbolScope>::_M_insert_aux` stays **733/746**
original-offset bytes, all 15 relocations. The genuine
`__uninitialized_copy_aux<normal_iterator<SymbolScope*>>` still emits
**145** bytes against the original **149**. In particular, simply
restoring FindCopy cannot explain the allocator vector pointer commit
register order or the four-byte EH landing alignment discrepancy.

The incomplete `SymbolTable::Enter` body is still non-exact and is not
part of the accepted manifest. We did not add inline assembly, synthetic
padding, source-only register pins, invented local functions or relocated
unverified addresses.

### Reproduction

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_symboltable_findcopy_true_tu_oct15n.py
```

Generated C++03/object files and strict per-case JSON are preserved in
`/tmp/vuc-binutils215/symboltable-findcopy-true-tunit-oct15n/` in
the existing Lima guest; accepted canonical function configs stay under
`vuc/config/`.

Avoid repeating lexical `FindCopy` placement variants; explore
*actual historical predecessor/source ownership* supported by original
symbol/file order, especially around the ordinary C++ EH landing
and Scope allocator templates.

The 298-function inventory remains **204 EXACT / 27 EXPERIMENTAL /
67 UNRESOLVED**. Entire linked VUC ELF byte identity is still
not established.

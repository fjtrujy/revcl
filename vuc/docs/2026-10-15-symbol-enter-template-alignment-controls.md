# VUC: SymbolTable::Enter alignment, STL vector-owner experiments (2026-10-15)

Target: original VUC 2004.1 Linux x86, original ELF at `0x0806a330` for `SymbolTable::Enter`, `0x0806a550` for `std::vector<SymbolScope>::_M_insert_aux`, `0x0805b040` for `std::vector<std::string>::_M_insert_aux`, and `0x0804ee10` for `std::vector<Type>::_M_insert_aux`. The two recovered GCC 3.3.3 prerelease snapshots and binutils 2.15 assembler are used. No inline assembly, machine-byte injection, synthetic relocation, accepted-source modification or matching-only pad.

## Rebase

Fetched upstream, rebased `vuc` onto newer `origin/main` commit `88ecfc3`. Original accepted sources and historical binary files preserved; 155 unrelated untracked files remain untouched.

## SymbolTable::Enter: identify the source-order and alignment differences

The provisional `vuc/src/SymbolTableUnit.cpp` Enter method generates **180 bytes** under `-O2 -frename-registers -falign-jumps=1`, compared with original **185**. The original tests the empty vector and returns at +0x2a, then has a real **five-byte compiler alignment sequence** at +0x2b..+0x2f before the node-allocation branch at +0x30. The non-aligned source emits that branch at +0x2b, with the rest of the node construction and hash bucket calculation largely identical until the last stores.

The original stores `node->next = oldHead` before `heads[bucket] = node`. Our provisional source expresses that order but GCC 3.3 moves the independent writes the other way around. This is source/compiler scheduling, not permission to add arbitrary NOPs or assembly.

`vuc/probes/evaluate_symbol_enter_alignment_store_oct15k.py` tests four real C++03 bucket insertion expressions (direct store, prior old-head value, bucket-pointer alias, human-written head-first store), with seven compiler alignment contexts motivated by the branch-target and loop assembly. **28 compilations** with original per-function length and original relocation sites enforced in the comparator. No 185-byte Enter or better 746-byte scope vector was found; the strongest retained Scope result remains **733/746**, **15 authentic relocations**. The aligned variants often invalidate the original size of the accepted 115-byte Find and 140-byte BeginScope methods in the same compilation unit.

A separate bounded targeted test of `-falign-jumps=8/16` and `-falign-loops=1/2/4/16`, always with `-falign-labels=1`, found the original Enter shape at **186 bytes** for jump=8 and **194 bytes** for jump=16, with **745-byte** Scope vector for loop=1/2 and 756/764-byte variants for larger loops. Neither source unit meets the strict original Enter or Scope lengths. Three of the four source orders were independently checked with these flags; none yielded the required lengths. The guest retains those objects at `/tmp/vuc-binutils215/symbol-j*-loop*-oct15k.o` and the exact per-candidate ledger at `/tmp/vuc-binutils215/symbol-enter-alignment-store-oct15k/results.json`. Do not repeat this parameter grid without new evidence.

## String-vector template: original size alone is not enough

The genuine `ListFile::Generate` reconstruction in `vuc/src/ListFileUnit.cpp` instantiates GCC 3.3.3 `std::vector<std::string>::_M_insert_aux`. Original function length **668 bytes** at `0x0805b040`. Normal `-O2 -frename-registers` produces **700 bytes**; `-falign-jumps=1` produces **651**. A constrained original branch/loop alignment check found `-falign-jumps=16 -falign-loops=2 -falign-labels=1` produces an original-length **668-byte** body. However, *authentic relocation resolution and original-offset byte comparison* gives only **213/668**, with **18 legitimate relocation sites**, so this is not a useful partial. The branch decisions, register ownership or true ListFile owner have not been recovered. The exact original symbol, size and PLT destinations were checked using `match_probe.py`; the temporary config/object remain in the guest at `/tmp/vuc-binutils215/string-vector-probe-oct15k.json` and `string-vector-align-j16-loop2-oct15k.o`.

## Type-vector helper: destructor-lifetime hypothesis rejected

The recovered 16-byte `Type` copy constructor from `vuc/src/TypeUninitializedCopy.cpp` is used to instantiate the original-era `std::vector<Type>::_M_insert_aux` template in a separate, explicit-instantiation probe. Original helper at `0x0804ee10` is **525 bytes**. Using the reconstructed type under `-O2 -frename-registers` gives **440 bytes**. The Type destructor could plausibly affect exception unwind code, so the driver `vuc/probes/evaluate_type_vector_destructor_oct15k.py` tests implicit, empty inline and out-of-line empty C++03 destructors under three evidence-motivated layouts.

- Implicit and inline-empty destructors both compile to **440 bytes**, or **413 bytes** with the targeted jump/loop alignment; neither matches original length.
- An out-of-line empty destructor creates a `Type::~Type` call relocation in the helper that is absent from the original ELF and is rejected by strict comparison. The candidate is **not eligible**, irrespective of superficial function size.
- Do not promote explicit-instantiation probes to accepted application translation units as though the originating application method were known.

The driver, generated object files and JSON ledger remain under `/tmp/vuc-binutils215/type-vector-dtor-oct15k/`; the standalone C++03 hypothesis is `vuc/probes/TypeVectorInsertionRealTypeOct15k.cpp`. No existing exact `Type` uninitialized-copy helper is changed.

## Next actions

1. Reconstruct the **actual** original 185-byte `SymbolTable::Enter` predecessor from original ELF semantics, especially the early-return jump arrangement and hash-bucket store order. Then revisit original translation-unit compiler state and the 13 Scope vector pointer-commit bytes. No further generic option permutations without a concrete new control-flow hypothesis.
2. Investigate other real-source-unit STL instantiations only when the initial size and relocations are original-compatible. Do not treat a 213/668 alignment coincidence as progress.
3. Preserve the strict inventory **204 EXACT, 25 EXPERIMENTAL, 69 UNRESOLVED** until another function satisfies complete length, bytes, literals and relocations.

Whole-ELF byte identity remains unproven.

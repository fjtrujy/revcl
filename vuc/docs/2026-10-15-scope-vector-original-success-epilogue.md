# VUC Scope insertion: original reallocation epilogue and SymbolTable.cpp ownership

Date: 2026-10-15. Branch: `vuc`.
Target: the immutable SHA-256-authenticated VUC 2004.1 ELF32 i386 executable.
Compiler: recovered GCC 3.3.3 Jan25 and Dec06 prerelease snapshots with the GNU as 2.15 wrapper in the existing Lima guest.

## Rebase and preservation

Started at `beeb510`. Fetched `origin`, observed one newer `main` commit, and cleanly rebased the 149 VUC-local commits onto `origin/main` at `104f165`; latest VUC HEAD after replay was `e8cc3db`. All 155 pre-existing unrelated untracked files were preserved. The original archive, root VCL project, toolchains, accepted ELF comparator and linker artifacts were not changed.

## Where the 13 bytes actually differ

The prior report correctly established the strict **733/746** original-offset partial for `std::vector<SymbolScope>::_M_insert_aux` at `0x0806a550`, with all **15** authentic relocation sites. Its generic mention of the exception/allocator region was imprecise.

Disassembly of the original executable and historical GCC object localizes *all* differences to **the successful vector-growth commit after the optional old-storage deallocation**, at offsets:

`+513, +520, +534, +535, +540, +541, +542, +543, +544, +545, +546, +547, +548`.

The original code from `0x0806a74a` (`+0x1fa`) has this effective write order:

```asm
mov edi, [ebp-0x83c]     ; new vector allocation capacity
mov edx, [ebp-0x83c]
mov ecx, [ebp-0x840]     ; new start
mov eax, [ebp-0x840]
shl edi, 9
mov ebx, [ebp-0x844]     ; new finish
mov [esi], ecx          ; _M_start
add edi, edx
lea ecx, [eax+edi*4]
mov [esi+4], ebx        ; _M_finish
mov [esi+8], ecx        ; _M_end_of_storage
```

The reconstructed GCC object instead performs equivalent loads with a different owner register allocation, and writes the vector's end pointer before its start:

```asm
mov edi, [ebp-0x83c]
mov ebx, [ebp-0x83c]
mov ecx, [ebp-0x844]
mov eax, [ebp-0x840]
shl edi, 9
mov edx, [ebp-0x840]
mov [esi+4], ecx
add edi, ebx
lea ecx, [eax+edi*4]
mov [esi], edx
mov [esi+8], ecx
```

The reallocation, copy, exception cleanup and deallocation code **outside this region already matches**. All 13 mismatches are in these original-offset register selections, load addresses and store-order instructions. This is a backend register-lifetime/scheduling difference, *not* a missing exception block, wrong allocator ABI, wrong container size, missing relocation, or wrong type.

## Genuine SymbolTable predecessors

The original ELF places `SymbolTable::Enter`, `Find`, `BeginScope`, `EndScope` before the `std::vector<SymbolScope>::_M_insert_aux` weak template. The original `SymbolTable.cpp` source-file record survives in `STT_FILE`; application DWARF statements do not.

`vuc/probes/evaluate_scope_real_symboltable_predecessors_oct15j.py` recompiles 15 normal 2004-era C++ source expressions for the incomplete Enter method: real name/key aliases, late node initialization, named hash temporaries and actual bucket-slot pointer/reference operations, followed by the existing accepted Find and BeginScope methods. The compiler flags are **fixed** to `-O2 -frename-registers -falign-jumps=1`, rather than sweeping arbitrary optimizations. Every valid vector instantiation remains **733/746**, same 15 relocations and the same 13 mismatches. `Find` stays **115/115** exact across all builds. Compiler sources, .o objects and per-case JSON records remain under `/tmp/vuc-binutils215/scope-symboltable-predecessors-oct15j/` in the guest.

The current Enter predecessor is **180 bytes** with the vector-correct `-falign-jumps=1` option versus the original **185-byte** Enter at `0x0806a330`. A five-byte alignment region follows the original early-return branch. But explicitly testing `-falign-jumps=16` generated **194 bytes** for Enter and **764 bytes** for the Scope vector insertion. Thus the mismatch cannot simply be cured with one TU-wide alignment switch. Recovering the true 185-byte predecessor is still important.

Historically plausible original iostream setup options `-fuse-cxa-atexit` and `-ffunction-sections` were additionally tested with the real `SymbolTableUnit.cpp` header. They do not change the 733/746 partial. As a bounded sanity check, disabling register renaming under the original size and jump-alignment gate yields only **561/746** under *both* GCC snapshots; a broad compiler-flag sweep is not justified by this evidence.

## A better original EndScope reconstruction (already exact)

The original `SymbolTable::EndScope()` at `0x0806a500` is exactly 27 bytes. It reads the `std::vector` finish pointer from `this + 4`, subtracts `0x804` (the size of a `SymbolScope`), stores the new finish, then tail-calls the original `MLHashTable::clear()` relocation. This is precisely how 2004-era `std::vector<SymbolScope>::pop_back()` compiles.

Previous source `vuc/src/SymbolTable.cpp` used an artificial class with an extra reserved word and a manually adjusted `SymbolScope*`. It happened to be byte-exact but did not explain the program's real data structure.

Replaced that standalone reconstruction with the actual `std::vector<SymbolScope> scopes_` and ordinary `scopes_.pop_back()`; separately added the same natural method to the real-order `vuc/src/SymbolTableUnit.cpp`. This is backed by original memory offsets, destructor behavior and compiler output, not just source aesthetics.

`vuc/probes/evaluate_scope_end_scope_original_unit_oct15j.py` checks this original owner with Jan25 and Dec06 GCC:
- `SymbolTable::Find`: **115/115**, 1 relocation.
- `SymbolTable::BeginScope`: **140/140**, 5 relocations.
- `SymbolTable::EndScope`: **27/27**, 1 relocation.
- `std::vector<SymbolScope>::_M_insert_aux`: **733/746**, 15 relocations.

This is the first test of these three accepted methods and the partial vector template with a *single coherent original-order source unit* that defines all three methods. The Enter member body is still provisional and **not accepted**. The pop_back idiom matches when compiled alone too, in `vuc/src/SymbolTable.cpp`, with both historical compiler snapshots under the original `-O2 -frename-registers` manifest flags.

The `EndScope` function was **already exact** before this batch; replacing its provisional source with natural original-owner C++ does **not** add an accepted function or increase the byte totals.

## Reproduction

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_scope_real_symboltable_predecessors_oct15j.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_scope_end_scope_original_unit_oct15j.py"
```

For each compiler snapshot, compile `vuc/src/SymbolTableUnit.cpp` with `-O2 -frename-registers -falign-jumps=1` and run the relocation-aware `match_probe.py` gate against `symboltable_find_probe.json`, `symbol_begin_scope_probe.json`, `symboltable_end_exact_probe.json`, and `scope_vector_insert_partial_oct15i.json`. Compile the independent `vuc/src/SymbolTable.cpp` with `-O2 -frename-registers` to verify its still exact 27-byte method.

## Next highest-value evidence

The remaining Scope register allocations are likely affected by source-unit compiler state or differences in the original GCC/backend build; the tested expression variations and original startup/section options did not change them. Further lexical rewrites of `Enter` or optimizer settings without new evidence risk repeating failed work.

Prefer (1) disassembling the original `SymbolTable::Enter` and recovering its *actual* 185-byte body so that all four original methods in the translation unit are authentic, (2) reconstructing the historical `MLHashTable` copy/destroy ownership that causes this weak template instantiation, and (3) inspecting final RTL before/after GCC register-renaming and scheduling in the real source unit, keeping compiler dump files outside Git. Preserve the partial until all **746** bytes and all original relocations authenticate; never force register allocations using assembly.

The whole linked VUC executable still has no validated byte-identical rebuild.

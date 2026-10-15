# VUC 2004.1: two exact STL vector functions from genuine translation units

Date: 2026-10-15. Branch: `vuc`. Original target: SHA-256-pinned VUC 2004.1 Linux i386 executable in its tracked upstream ZIP. Compiler: original-era GCC 3.3.3 prerelease January 25, 2004 and independently recovered December 6, 2003 snapshots; GNU assembler 2.15.

## Why this batch differs

Instead of repeatedly modifying equivalent expressions in near-exact callbacks, examine *which original C++03 source unit instantiates a library template*. In VUC, the compiler emits `std::vector<T>::_M_insert_aux` as a separate weak function when an authentic application `push_back` instantiates it. This reproduces complete machine code with the original 2004 libstdc++ semantics, including exception paths, without handwriting or decompiling the 265- and 444-byte functions themselves.

The original `.symtab` retains `Instruction.cpp` source-file provenance, and original ELF code order places `InstructionList::Add` before the vector insertion template. `InlineAsm.cpp` likewise contains the original inline-assembly symbol binder and an adjacent vector insertion template. Although the original C++ application DWARF did not survive, source ordering, named symbol/relocation references and compiler-generated template ownership give direct evidence.

The previously accepted application source units already reconstruct the authentic instantiation triggers:
- `vuc/src/InstructionListLineCommentUnit.cpp`: `instructions_.push_back(instruction)`.
- `vuc/src/InlineAsmUnit.cpp`: `stmt->variables.push_back(variable)`.

No new handwritten implementation of the STL function or raw machine code was added.

## Newly byte-exact functions

| Original function | Address | Original length | Matching machine bytes | Authenticated ELF relocations | Source and flags |
| --- | --- | ---: | ---: | ---: | --- |
| `std::vector<Instruction*>::_M_insert_aux` | `0x080519d0` | 265 | **265/265** | 5 | `InstructionListLineCommentUnit.cpp`, `-O2 -frename-registers -falign-jumps=1` |
| `std::vector<AsmVariable>::_M_insert_aux` | `0x08051290` | 444 | **444/444** | 9 | `InlineAsmUnit.cpp`, `-O2 -frename-registers` |

Both functions compare byte-identically against the original ELF with the historical January and December GCC snapshots, including the correct emitted symbol names, full original function lengths and original PLT / application relocation targets.

The Instruction template calls `memmove` three times and the original `__default_alloc_template<true,0>` allocate/deallocate functions (five total sites). Without the original `-falign-jumps=1` code-placement hypothesis its emitted length is 276 bytes rather than the original 265. With the flag, **the entire function matches**.

The AsmVariable template's nine call relocations resolve to the original `std::__uninitialized_copy_aux<AsmVariable*>` instantiation, original `__default_alloc_template` allocate/deallocate and genuine `__cxa_begin_catch`, `__cxa_rethrow`, `__cxa_end_catch`, and `_Unwind_Resume` entrypoints. It matches all **444** original bytes with ordinary `-O2 -frename-registers`.

These are **709 newly accepted instruction bytes**. The exact function inventory advances **202 -> 204**, accepted instruction bytes **15,146 -> 15,855**.

## Near-exact third STL function: Scope vector insertion

A genuine `SymbolTable::BeginScope` source unit `vuc/src/SymbolTableBeginScope.cpp` instantiates:
`std::vector<MLHashTable<MLGameString,SymbolTableEntry*,MLHashFun<MLGameString>,513>>::_M_insert_aux`.

Original `0x0806a550`, length **746**. GCC January and December snapshots with `-O2 -frename-registers -falign-jumps=1` both emit a **746-byte** function with **733/746** original-offset matching instruction bytes, **15 authenticated relocation sites**, no missing original literal bytes, and exactly 13 mismatching offsets:
`+513, +520, +534, +535, +540, +541, +542, +543, +544, +545, +546, +547, +548`. This remains **EXPERIMENTAL**, not accepted as exact.

Source wrapper `vuc/probes/ScopeVectorInsertionOriginalUnitOct15i.cpp` uses the actual accepted `SymbolTableBeginScope.cpp` implementation, not a synthetic template rewrite; config `vuc/config/scope_vector_insert_partial_oct15i.json` records all original function/PLT relocation targets. Recompiling the fuller provisional `SymbolTableUnit.cpp` with real Enter/Find/BeginScope methods still produces 733/746, so the current mismatch needs deeper original allocator exception-path ownership, not yet another arbitrary flag sweep.

The inventory after adding this strict partial is **204 EXACT, 25 EXPERIMENTAL, 69 UNRESOLVED**, 298 named functions.

## Real translation-unit coexistence controls

Two independent bounded Python drivers under `vuc/probes` measure *multiple* functions from the **same** generated historical ELF32 object instead of just isolated methods:

- `evaluate_instruction_true_tu_coexistence_oct15i.py`: assembles actual reconstructed InstructionList Add/AddB/AddL/AddLineComment/GenerateImage/GenLabel/GenTmp/Print, Instruction::ToString/allocator and arena lifecycle in original address order, comparing 13 function configurations under two historically motivated options and **both** compiler snapshots. Coexistence confirms simultaneously exact Add/Print/etc. components, and isolates remaining arena `__tcf_1` at **45/46**, mismatch `pop %ecx` versus compiler scratch register.
- `evaluate_printer_true_tu_coexistence_oct15i.py`: composes `Print(ASTFunCall*)`, real `Printer_UglyPrint`, the original std::iostream initializer and genuine AST node-list template instantiations. With historical startup options the module printer remains **40/40**, wrapper **31/31**, and list printers **80/80** simultaneously, but ASTFunCall remains **92/98**. Plain `-O2` yields **96/98** ASTFunCall but regresses the other startup functions. This indicates incomplete original predecessor or inlining context; original large AST statement/expression printers remain unreconstructed.

The complete per-method report JSON and generated C++03/object files are retained in the guest:
`/tmp/vuc-binutils215/instruction-true-tunit-oct15i/` and
`/tmp/vuc-binutils215/printer-true-tunit-oct15i/`.

This is strong evidence that exactness must be assessed *per function and against the full original source-unit neighborhood*. Separate original compiler options remain unproven at the TU-wide level; simultaneous exact subsets do not establish whole-TU identity.

## Reproduction

Run in the existing Lima VM:

```sh
R=/path/to/revcl/vuc
for compiler in jan25 dec06; do
  limactl shell revcl-oracle -- /tmp/vuc-binutils215/g++-${compiler}-as215 \
    -O2 -frename-registers -falign-jumps=1 \
    -c "$R/src/InstructionListLineCommentUnit.cpp" \
    -o "/tmp/vuc-binutils215/instruction-vector-${compiler}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/instruction-vector-${compiler}.o" \
    --config "$R/config/instruction_vector_insert_exact_oct15i.json"

  limactl shell revcl-oracle -- /tmp/vuc-binutils215/g++-${compiler}-as215 \
    -O2 -frename-registers \
    -c "$R/src/InlineAsmUnit.cpp" \
    -o "/tmp/vuc-binutils215/asm-vector-${compiler}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/asm-vector-${compiler}.o" \
    --config "$R/config/asm_variable_vector_insert_exact_oct15i.json"

  limactl shell revcl-oracle -- /tmp/vuc-binutils215/g++-${compiler}-as215 \
    -O2 -frename-registers -falign-jumps=1 \
    -c "$R/probes/ScopeVectorInsertionOriginalUnitOct15i.cpp" \
    -o "/tmp/vuc-binutils215/scope-vector-${compiler}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/scope-vector-${compiler}.o" \
    --config "$R/config/scope_vector_insert_partial_oct15i.json"
done
```

No ELF, archive, original VCL root files, unrelated untracked files, historical compiler binary, relocation matcher or accepted prior function implementation was modified.

## Next source-unit reconstruction targets

1. Reconstruct real `SymbolTable.cpp` and `MLHashTable` exception/copy destructor owner context around the Scope template's 13 residual bytes. The current original-sized 733/746 result is an excellent acceptance candidate.
2. Use naturally instantiated `std::vector<std::string>` and `std::vector<Type>` STL routines in their genuine original application translation units. Do not mistake wrong-length overlap for original-length progress.
3. Continue authentic `Instruction.cpp` and `Printer.cpp` methods, but avoid repeating previously exhausted GCC flag or register-pinning matrices.

Whole-linked-executable byte identity remains **not established**. Exact function, initialized data and BSS checks are separate gates.

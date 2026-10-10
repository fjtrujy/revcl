# Original ASTNode arena global initialization wrapper

Date: 2026-10-10; branch: `vuc`.

`_GLOBAL__I__ZN7ASTNode3memE` at original ELF virtual address
`0x0804a920` is now **31/31 original instruction bytes** using
the already byte-exact `vuc/src/ASTNodeArenaCleanup.cpp` source
fragment, the 25 January 2004 GCC 3.3.3 prerelease compiler
and historical GNU as 2.15. Its `R_386_PC32` call to
`__static_initialization_and_destruction_0(int,int)` resolves
to **`0x0804a7f0`** in the original ELF.

The compiler's source hypothesis is
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`.
The `-ffunction-sections` choice yields a relocatable call to
the helper in the candidate object, enabling correct strict
rebasing to the original linked address. This **does not prove**
that the original compiler used this specific option.

The underlying `ASTNode::mem` arena object, its 64-KiB chunk
allocation method and its linked-list cleanup callback were
already accepted. This initializer wrapper is compiler-generated
scaffolding for that same subsystem, not an additional original
application algorithm.

The larger original initializer helper at `0x0804a7f0`
is **now also 138/138 byte-exact**, with all **10** original
ELF relocations resolved. The ordinary C++ `ArenaCleanup`
constructor allocates 0x10008 bytes, initializes the linked
arena cursor at chunk+4 and null predecessor at chunk+0x10004,
then registers teardown and initializes `std::ios_base::Init`.
The source order of the two chunk stores was necessary to obtain
the original compiler's register allocation. This reconstructed
initializer is compiler-generated lifecycle code, not a distinct
application algorithm or proof of original translation-unit layout.

The new strict gate is **122/122 original function-symbol records,
6,594/6,594 instruction bytes, 287 resolved relocations,
36 verified referenced literal bytes**. The unit test suite
passes 51 tests. The original symbol ledger contains 122 EXACT,
10 EXPERIMENTAL and 166 UNRESOLVED out of 298 named functions.

The entire original ELF is **not byte-exact**. Function-level
matching is a necessary but insufficient subset of the full
executable reconstruction.

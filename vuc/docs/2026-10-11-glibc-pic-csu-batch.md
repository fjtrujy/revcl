# Byte-exact glibc 2.3.2 fini-array destructor in original VUC

Date: 2026-10-11. Branch: `vuc`.

## Newly accepted original function

**`__libc_csu_fini` at `0x08071fd0`, 80/80 instruction bytes,
five independently resolved PIC relocations.**

The canonical VUC 2004.1 ELF contains glibc 2.3.2 startup objects,
including a `__libc_csu_fini` function at this original linked address.
The original retained DWARF compiler provenance for libc startup
objects identifies the December 6, 2003 GCC 3.3.3 prerelease. We
recovered and built the December compiler's C frontend (not only
the previously available C++ frontend) in the existing Linux VM.
No historical source or binary is modified in the repository.

The provisional `vuc/src/LibcCsu.c` uses *ordinary C*, not C++ and not
handwritten assembly. It expresses the historical array traversal:

```c
unsigned int count = __fini_array_end - __fini_array_start;
while (count-- > 0)
    (*__fini_array_start[count])();
_fini();
```

The `__fini_array_start/end` declarations have hidden ELF visibility,
consistent with their original linker-resolved symbols. The original
symbol table confirms both as zero-sized `SHN_ABS` at `0x08077000`.
The original `_GLOBAL_OFFSET_TABLE_` symbol is at `0x0808a668`,
and `_fini` is at `0x08072060`.

Both date-matched **2003-12-06** and **2004-01-25** GCC 3.3.3 C compiler
frontends, with historical GNU **as 2.15**, produce an identical
**80-byte exact match** under `-O2 -fPIC`. Adding
`-frename-registers` changes the exact loop's register allocation to
only **72/80 matching bytes**. Ordinary C loop variants were tested;
the original postdecrement-while expression is the exact one.

## Full PIC relocation authentication

This result did **not** come from patching linked bytes or embedding
original instructions. The existing ELF32 named-symbol strict matcher
has been extended to resolve:

- `R_386_GOTPC` (type 10): `GOT + addend - relocation_place`;
- `R_386_GOTOFF` (type 9): `symbol + addend - GOT`;
- `R_386_PLT32` (type 4): `symbol + addend - relocation_place`.

PIC GOT mode requires the exact authenticated original `_GLOBAL_OFFSET_TABLE_`
address and verifies **each explicitly provided named original PIC
relocation symbol** in the canonical ELF `.symtab` (including zero-sized
absolute init/fini array symbols). Inventing GOT bases, array addresses
or `_fini` destinations is rejected. The candidate receives no
unchecked section-wide rebasing or relocation score allowance.

The strict manifest `vuc/config/libc_csu_fini_pic_probe.json` records
the source, original address, `-xc -O2 -fPIC` compiler invocation,
GOT base, original array addresses, and original `_fini` address.
With those assignments, the candidate passes **80/80 bytes** and
**5/5 relocation resolutions**. Regression tests validate PIC
relocation arithmetic and reject a wrong GOT, wrong array address,
and wrong `_fini` target.

## What remains unverified

The adjacent `__libc_csu_init` function, originally **84 bytes**,
is represented provisionally by the same source file but **does not
match yet**. The January C compiler's simplest source emits a
94-byte body; `-falign-jumps=1` gives 81 bytes. This non-exact
function is not in the accepted manifest. The other original
startup/linker objects are also not reconstructed.

This checkpoint brings the complete named-function strict gate to
**163/163 exact original records**, **11,007/11,007 instruction
bytes**, **414 resolved relocations**. The remaining 298-symbol
inventory is **163 EXACT, 9 EXPERIMENTAL, 126 UNRESOLVED**.
These are original *function-symbol records*, not an entire-ELF
matching percentage. A byte-for-byte identical complete VUC
executable is still mandatory and has not been produced.

## Reproduce

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-163
```

Using the independently recovered December 2003 *C compiler* should
also yield the same original `__libc_csu_fini` bytes.

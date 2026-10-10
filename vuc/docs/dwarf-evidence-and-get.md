# Actual DWARF content in the original VUC 2004.1 ELF

## Answer: does the binary contain useful VUC class debugging information?

**No.** The original ELF contains valid DWARF v2 sections, but none of the
four surviving DWARF compilation units comes from VUC's C++ application
translation units. This is separate from the rich ELF `.symtab`, which
contains unmangled and mangled function names and 22 original VUC `.cpp`
filenames.

The original ZIP-backed ELF contains these debug sections:

| Section | Bytes |
| --- | ---: |
| `.debug_info` | 2,696 |
| `.debug_abbrev` | 312 |
| `.debug_line` | 611 |
| `.debug_str` | 1,729 |
| `.debug_aranges` | 120 |
| `.debug_pubnames` | 37 |

Its **four** `DW_TAG_compile_unit` roots are:

| Unit name | Producer |
| --- | --- |
| `../sysdeps/i386/elf/start.S` | GNU AS 2.14.90.0.7 |
| `init.c` | GNU C 3.3.3 20031206 (prerelease) (Debian) |
| `crti.S` | GNU AS 2.14.90.0.7 |
| `crtn.S` | GNU AS 2.14.90.0.7 |

Their `DW_AT_comp_dir` paths identify the glibc 2.3.2 startup build
under `/home/drow/deb-cvs/nptl/glibc-2.3.2.ds1/`.
The decoded DWARF line table only maps startup routines such as `start.S`
and `crti.S`. `.debug_pubnames` contains `_IO_stdin_used`, not VUC
classes. There is no `MLGameString.cpp`, `Instruction.cpp`,
`MLGameStringTable`, `AsmOutput.cpp` or application class compilation
unit in the retained DWARF.

**This does not mean the ELF was stripped of its normal symbol table.**
The ELF `.symtab` preserves 298 individually named text function records,
including C++-mangled symbols, addresses, sizes, local/global bindings,
and 57 `STT_FILE` records. It does *not* preserve arbitrary C++ statement
expressions, most local variable names, original ASTs, or complete class
member/type definitions.

### Toolchain implications

`GNU AS 2.14.90.0.7` from DWARF identifies the assembler that produced
the **glibc startup assembly**, not necessarily the assembler used to
assemble VUC application objects. Our separate GNU as 2.15 recovery remains
a **compatibility candidate** that reproduces original alignment bytes
for `MLHashFun<char const*>::Hash()`, not a proven complete toolchain.
Likewise, the `20031206` compiler string in `init.c` is a *glibc object*
producer; the application compiler must be investigated separately using
`.comment`, generated instructions, and other provenance.

### Reproducible inventory

The standard-library-only script parses DWARF v2 compilation-unit DIE
headers and the necessary abbreviation/string forms **directly from the
original ZIP-backed ELF**, without replacing or editing that file:

```sh
python3 vuc/scripts/dwarf_inventory.py --write
python3 -m unittest discover -s vuc/tests -v
```

The checked-in `vuc/config/dwarf_units.json` is regenerated deterministically
and regression-tested. The parser is deliberately a *compilation-unit root
inventory*, not a full DWARF type decoder. Adding a full DWARF C++ type
decompiler would not recover absent application DIEs.

For independent verification under the Debian VM:

```sh
limactl shell revcl-oracle -- readelf --debug-dump=info \
  /path/to/revcl/archive/upstream/vuc/downloads/X86linux_vuc2004_1/vuc
```

## Reconstruction decision

Continue reconstructing VUC by translating original disassembly to C++
source hypotheses, guided by ELF function names, symbol sizes and relocation
targets, `STT_FILE` provenance, static data, and the GCC 3.3.3 + GNU as 2.15
code-generation oracle.

Every promoted function must still pass the existing **100% byte-exact
function regression gate**. No partial matching is accepted, and matching
individual functions does not prove exactness of the final linked ELF.

### Current adjacent investigation

`MLGameStringTable::Get()` has a provisional source hypothesis using a
513-bucket table, static local initialization, destructor registration,
and `-fuse-cxa-atexit`. Its compiled body is **106 bytes**, the original
length, with **92/106 matching code bytes** and nine resolved relocations.
Changes to the two trailing counters' initialization order improve this to
94/106 but do **not** complete the function. The compiler-generated
`__tcf_0` cleanup callback separately matches **22/22 bytes**, with two
resolved relocations. It is *not* accepted into `vuc/src/` because the
surrounding table implementation is still experimental and the symbol
`__tcf_0` also occurs elsewhere in the ELF.

`MLGameStringTable::Get()` is therefore listed as EXPERIMENTAL alongside
`AsmOutput_WritePrologue` in the exhaustive reconstruction ledger:
**25 EXACT, 2 EXPERIMENTAL, 271 UNRESOLVED** of 298 function symbols.
Neither function is promoted to the byte-exact accepted list.

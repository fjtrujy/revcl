# VUC toolchain investigation — initial evidence

## Verified binary identity

Canonical source: `archive/upstream/vuc/downloads/X86linux_vuc2004_1.zip`
(member `X86linux_vuc2004_1/vuc`). Verify through
`python3 vuc/scripts/decomp.py inventory`; the command rejects a SHA mismatch.

The ELF is 32-bit i386, with 35 sections and a 165,440-byte `.text` section.
The static symbol table has 716 named entries, of which 298 are function
symbols in `.text` at 298 different addresses. This is **not** the final count
of all compiled routines: assembly/runtime thunks, anonymous code and any
zero-sized symbols must still be classified. The `.text` area should not all be
counted as user-written VUC functions.

Its dynamic imports require:
- `libstdc++.so.5`
- `libm.so.6`
- `libgcc_s.so.1`
- `libc.so.6`

## Compiler evidence

The `.comment` section contains 28 visible GCC tags:
- 4 copies of `GCC: (GNU) 3.3.3 20031206 (prerelease) (Debian)`
- 24 copies of `GCC: (GNU) 3.3.3 20040125 (prerelease) (Debian)`

These tags may reflect objects from multiple historical compiler builds, but
the ELF alone does not yet assign them confidently to specific original source
files. `.comment` occurrence order is not a proven translation-unit mapping.
Do not unconditionally choose a single date or infer `-O2`, frame-pointer
behavior, register renaming, exception flags or linker options from the tags.

The ELF retains `.symtab`, `.strtab`, `.debug_info`, `.debug_line`,
`.debug_aranges`, and `.debug_str`. Disassembly confirms recognizable
C++/iostream call sites and modern-for-2004 GCC-generated prologues.

## Current host limitations

The inspected macOS host reports Apple Clang 17 as `gcc` and Apple LLVM
`objdump`; no `docker` or `podman` executable was found on PATH. The
VCL's ignored `tools/rh9/` directory and local `.venv/` are absent.
Consequently, no original-compiler build comparison has yet been performed.

## V1 reproducibility checklist

1. Locate/build an immutable GCC 3.3.3 Debian historical toolchain and
   matching binutils, assembler, linker, i386 libc and libstdc++.so.5.
   Capture exact package provenance and SHA-256 before treating it as canonical.
2. Run small test translation units using historical C++ constructs (iostream
   insertion, std::vector, global constructors/destructors, RTTI, exceptions)
   with several candidate flags. Compare section layouts, `.eh_frame`, register
   choices, function boundaries and object symbol/weak emission against known
   original addresses.
3. Use retained symbols/debug information and `.comment` metadata to propose
   original object boundaries. Distinguish claims from proven boundaries.
4. Only after the compiler has passed probes, add build execution and
   relocation-aware matching for candidate functions and data. Keep debug/symbol
   fidelity as separate score components from executable code bytes.
5. Do not report a binary matching percentage until the scorer is validated
   against exact and deliberately perturbed test objects.

## Reproduction commands

```sh
python3 vuc/scripts/decomp.py inventory --write
python3 vuc/scripts/decomp.py info 0x08049b40
python3 vuc/scripts/decomp.py asm 0x08049b40
python3 -m unittest discover -s vuc/tests -v
```

The `inventory --write` operation regenerates `target.json`,
`functions.tsv`, and `sections.tsv` deterministically from the ZIP.

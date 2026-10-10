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
An existing x86_64 Lima VM, `revcl-oracle` (Debian 12), is available and
supports execution of 32-bit ELF tools after installing the guest's
`libc6-i386` and `libc6-dev-i386` packages. This changes only that guest's
system packages and does not install the old compiler into its `/usr`.

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
5. Do not report a whole-binary matching percentage until the full scorer is
   validated against exact and deliberately perturbed test objects.

## Reproduction commands

```sh
python3 vuc/scripts/decomp.py inventory --write
python3 vuc/scripts/decomp.py info 0x08049b40
python3 vuc/scripts/decomp.py asm 0x08049b40
python3 -m unittest discover -s vuc/tests -v
```

The `inventory --write` operation regenerates `target.json`,
`functions.tsv`, and `sections.tsv` deterministically from the ZIP.

## Compiler fingerprint probes

The period-compatible C++ probe at `vuc/probes/compiler_fingerprint.cpp`
exercises iostream insertion, STL containers, virtual methods, static object
initialization and C++ exception handling. The dedicated runner compiles six
candidate option sets (`-O0`, `-O1`, `-O2`, `-O2 -frename-registers`,
`-O2 -fomit-frame-pointer`, `-Os`) with an **explicitly supplied compiler**:

```sh
python3 vuc/scripts/compiler_probe.py --compiler /path/to/g++ --out /tmp/vuc-probes
```

The resulting `manifest.json` includes compiler identity, commands, source
hash, candidate object hashes, ELF32-i386 identification and compilation
outcomes. These are fingerprints for future comparisons, **not** proof of the
original compiler flags or a whole-binary matching score. Apple Clang can
smoke-test the harness but must never be accepted as the historical compiler.

## Compatibility compiler bootstrap (not the exact original)

The original January 2004 compiler signature corresponds to Debian's
`gcc-3.3` source package `1:3.3.3ds3-0pre3`, as confirmed by its
[historical Debian bug record](https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=203835).
Debian's [public snapshot archive](https://snapshot.debian.org/) only has
main Debian FTP imports from 2005 onward. An exact i386 package for this
January 2004 prerelease was **not** recovered in this session.

A separately verified **GCC 3.3.5** (`Debian 1:3.3.5-13`) compatibility
toolchain is instead available from Debian's historical archive. All six
package SHA-256 values and the package URLs are pinned in
`vuc/scripts/bootstrap_compat_gcc.sh`. This compiler is deliberately labeled
noncanonical: it cannot prove code-generation equivalence to either VUC
GCC 3.3.3 prerelease. It is useful for building genuine ELF32-i386 objects,
exercising matching tools and testing compiler-independent hypotheses.

From the host:

```sh
limactl start revcl-oracle
limactl shell revcl-oracle -- sh -c 'sudo apt-get update && sudo apt-get install --no-install-recommends -y libc6-i386 libc6-dev-i386 binutils'
limactl shell revcl-oracle -- sh /path/to/revcl/vuc/scripts/bootstrap_compat_gcc.sh /tmp/vuc-gcc335
limactl shell revcl-oracle -- python3 /path/to/revcl/vuc/scripts/compiler_probe.py --compiler /tmp/vuc-gcc335/g++-compat --out /tmp/vuc-gcc335/probe-results
```

The last command produced six of six valid ELF32-i386 relocatable objects in
the VM. The wrapper forces 32-bit assembly (`-Wa,--32`) and uses the extracted
GCC 3.3.5 C++ headers; it does not change the VCL reconstruction. Scratch
objects and downloaded compiler packages remain under `/tmp/vuc-gcc335/`.

## First strictly resolved code comparison

The `AsmOutput_WritePrologue` function at `0x08049b40` is 30 bytes and
references a banner string at `0x080720a0` plus an iostream insertion
through the PLT at `0x080497bc`. These addresses were observed in the
original VUC disassembly and read-only data dump.

```sh
# Inside the existing revcl-oracle VM:
/tmp/vuc-gcc335/g++-compat -O2 -c \
  /path/to/revcl/vuc/probes/asm_output_hypothesis.cpp \
  -o /tmp/vuc-gcc335/asm_output.o
python3 /path/to/revcl/vuc/scripts/match_probe.py \
  --object /tmp/vuc-gcc335/asm_output.o
```

The first tested hypothesis, built with noncanonical GCC 3.3.5 `-O2`,
has **22 of 30 instruction bytes matching**, after resolving both
`R_386_32` and `R_386_PC32` relocations, and **92 of 92 matching bytes**
in the emitted literal section. It is **not exact**: eight instruction bytes
differ because the old GCC loads the receiver and writes the arguments in
a different order. The `-O2 -frename-registers` variant also gave 22/30;
`-O1`, `-Os`, and `-O2 -fomit-frame-pointer` produced different function
lengths (29, 20, and 28 bytes). These observations are *not* sufficient to
infer VUC's original flags.

The comparator deliberately rejects unsupported relocation types, unknown
symbol addresses, literal-data mismatches and different function sizes.
This is a scoped probe, **not** the full VUC relocation-aware scorer.

## Candidate-scoped batch auditing

The matching probe now locates a requested function using the ELF symbol
table, including its section and offset, and processes only relocations that
fall within that symbol's body. Compiling the prologue hypothesis alongside
an unrelated second function in the **same object file** produced the same
22/30 comparison result, demonstrating that other functions do not change
the selected comparison's scope.

A JSON job manifest can list multiple candidate objects and function
configurations; execute it with `python3 vuc/scripts/audit.py --jobs FILE`.
The result gives candidate-function totals, fully exact function count,
code bytes matched within these named candidates, and literal bytes checked.
It explicitly returns `whole_binary_score: null` and rejects duplicate
function entries or invalid original symbol identities.

The next step is resolving additional symbol/relocation cases and mapping
original translation-unit boundaries before introducing a meaningful
whole-binary scoring command. The exact GCC 3.3.3 compiler remains missing;
the GCC 3.3.5 comparison is useful diagnostic evidence, not a replacement.

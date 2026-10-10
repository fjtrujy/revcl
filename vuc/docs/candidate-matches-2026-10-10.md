# VUC code-generation verification — 10 October 2026

> This records the *initial four-function baseline*, before per-source compiler
> flag hypotheses and exact acceptance were introduced. For the authoritative
> **11-function exact-only regression set**, see `exact-match-contract.md`.

## Setup and scope

Original target: the SHA-256-verified `X86linux_vuc2004_1/vuc` member
inside the tracked upstream ZIP.

Compiler candidate: recovered upstream GCC 3.3.3, CVS date 25 January 2004,
built as a Linux-hosted i686 cross compiler in the existing `revcl-oracle`
Lima VM. The wrapper is
`/tmp/vuc-gcc333-jan25/g++-jan25-candidate`, which uses separately extracted
GCC 3.3.5-era libstdc++ headers until the original Debian `0pre3` headers
are recovered. Compiler output is genuine ELF32 i386; all probes use `-O2`.

The four sources below are **reverse-engineering hypotheses** based on
the original ELF symbols and machine code, not verified original class
declarations. Function-level equality demonstrates code-generation
compatibility for those functions only.

## Candidate verification results

| Original function | VUC address | Size | Exact bytes | Relocations | Result |
| --- | --- | ---: | ---: | ---: | --- |
| `ListFile::GetNumLines()` | `0x0805aff0` | 18 | **18** | 0 | **Exact** |
| `Instruction::ToString()` | `0x08051900` | 9 | **9** | 1 | **Exact** |
| `MLGameString::c_str() const` | `0x08071800` | 13 | 10 | 0 | Partial |
| `AsmOutput_WritePrologue(std::ofstream&)` | `0x08049b40` | 30 | 22 | 2 | Partial |

Two **distinct original functions** are exact, covering 27 function code
bytes. Across these four explicitly named hypotheses, **59 of 70** checked
instruction bytes match. `AsmOutput_WritePrologue` independently verifies
all 92 bytes of its mapped string literal.

- `ListFile::GetNumLines` emits pointer-end minus pointer-begin divided by
  four, consistent with `std::vector<const char*>::size()` at offset zero
  in the class.
- `Instruction::ToString` emits the matching 9-byte frame + tail jump;
  its `R_386_PC32` relocation resolves to
  `MLGameString::c_str() const` at `0x08071800`.
- `MLGameString::c_str()` has the correct 13-byte body and memory offsets,
  but GCC currently chooses different registers at three byte positions.
- `AsmOutput_WritePrologue` still differs in register assignment and
  instruction ordering. We have not inferred the original source precisely.

**These are not whole-program progress percentages.** Runtime/linker code,
unselected functions, global data, unwind/debug sections, and other bytes
remain outside the measured scope.

## Reproduce all four comparisons

After running `vuc/scripts/bootstrap_jan25_gcc.sh` in the Lima VM:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-gcc333-jan25/g++-jan25-candidate \
  --out /tmp/vuc-gcc333-jan25/candidate-suite
```

The selected source and reference manifests are enumerated in
`vuc/config/selected_candidates.json`; object output and the
`candidate_summary.json` report stay in VM scratch storage. Source files
are compiled once even when they contain multiple matched functions.
The runner rejects duplicate function identities, out-of-repository paths,
failed builds, unrecognized relocations, and disagreement with the
original ELF symbol name/address/size. Original archives remain untouched.

```sh
python3 -m unittest discover -s vuc/tests -v
```

## Debian GCC package provenance and patches

Debian's contemporary January 2004 package acceptance announcement identifies
`gcc-3.3 (1:3.3.3ds3-0pre3)`, including its i386 `g++-3.3` and
`libstdc++5-3.3-dev` packages:

https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=212912

An independently archived original Debian GCC build report, produced with
the **25 January 2004** compiler, gives the Debian configure arguments
and patch names:

https://www.mail-archive.com/debian-gcc@lists.debian.org/msg09389.html

The report lists `gcc-version` (appending “Debian” to the version),
`gcc-names`, `libtool-rpath`, `libstdc++-pic`,
`libstdc++-doclink`, `test-summary`, `libffi-*`,
`mips-branch-fix`, `alpha-ieee`, `autoreconf`, and language/install
packaging changes. The listed descriptions do **not** establish a modified
i386 instruction selector or register allocator; neither do they prove
that all i386-relevant downstream changes have been excluded.

The original build report records binutils
`2.14.90.0.7-4` and libc development version `2.3.2.ds1-11` for an
**alpha** build. These demonstrate the period environment but **are not
proof** of identical i386 versions or of the VUC author's own build flags.

**Still unresolved:** the authentic `0pre3` source diff, original compiler
binary and 2004 libstdc++ headers; original VUC per-translation-unit flags,
linker input ordering, and complete binary layout. These are required before
claiming a fully authenticated toolchain or whole-file byte-exact matching.

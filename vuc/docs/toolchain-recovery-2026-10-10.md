# GCC 3.3.3 January 2004 recovery evidence

Target VUC ELF .comment compiler labels:
- `GCC: (GNU) 3.3.3 20031206 (prerelease) (Debian)` (4 copies)
- `GCC: (GNU) 3.3.3 20040125 (prerelease) (Debian)` (24 copies)

## Identified original Debian package

Debian's January 26, 2004 package acceptance notice names
`gcc-3.3` source version `1:3.3.3ds3-0pre3`, with the following i386 binaries:
- `gcc-3.3_3.3.3-0pre3_i386.deb`
- `g++-3.3_3.3.3-0pre3_i386.deb`
- `cpp-3.3_3.3.3-0pre3_i386.deb`
- `libstdc++5_3.3.3-0pre3_i386.deb`
- `libstdc++5-3.3-dev_3.3.3-0pre3_i386.deb`

Original source archive names were `gcc-3.3_3.3.3ds3.orig.tar.gz`,
`gcc-3.3_3.3.3ds3-0pre3.diff.gz`, and
`gcc-3.3_3.3.3ds3-0pre3.dsc`.

Primary contemporary record:
https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=212912

The exact Debian package files are not present at the current
`archive.debian.org/debian/pool/main/g/gcc-3.3/` URL and are absent
from the available `snapshot.debian.org` version index.
The Internet Archive URL variants tested also returned HTTP 404.
**No original Debian .deb, .dsc or downstream diff was authenticated.**

## Verified near-exact upstream source

The GCC Git mirror preserves `releases/gcc-3.3` history. The commit
immediately preceding the historical 2004-01-25 16:09:13 UTC test snapshot is:

`5b4de06ecdb5c4a1c232325aa293bd7b5f33d94f`

Commit date: 2004-01-25 15:08:20 UTC.
Source: https://github.com/gcc-mirror/gcc/commit/5b4de06ecdb5c4a1c232325aa293bd7b5f33d94f

GitHub archive (SHA-256):
`cc34f2966ceeb89f9e5687af6ce03c3ae686e53d03667a9fe77fd45e750be611`

`gcc/version.c` in that commit explicitly says:
`3.3.3 20040125 (prerelease)`.

The second VUC tag, `20031206`, corresponds to an earlier Debian
prerelease. A Debian archive announcement names source version
`gcc-3.3_3.3.3ds0-0pre0` and its i386 packages:
https://lists.debian.org/debian-gcc/2003/12/msg00030.html
The corresponding GCC branch commit was identified as
`82bacd9e1bfbbb138c1401ec85d4a7dd597836d5` (2003-12-06 22:19:30 UTC).
This is a historical lead only; its exact Debian build remains unverified.

The GCC test-results email for the corresponding original Debian compiler
reports `LAST_UPDATED: Sun Jan 25 16:09:13 UTC 2004`:
https://lists.debian.org/debian-gcc/2004/02/msg00022.html

**This is source-date matching, not proof of the same Debian patch level.**

## Historical build dependencies

GCC's C++ parser from January 2004 cannot be generated with modern
Bison 3.x (the grammar's midrule typing is rejected). Debian's archived
`bison_1.875d-1_i386.deb` works with the old parser skeletons:

URL: https://archive.debian.org/debian/pool/main/b/bison/bison_1.875d-1_i386.deb

SHA-256:
`93a3264eed76aa85455cf6385fcad5f161112ad17535b0b52be889906d94df41`

When using an unpacked binary, set
`BISON_PKGDATADIR=<scratch>/old-bison/usr/share/bison`.

The GCC source's historical `obstack_ptr_grow` macro also assumes a cast
expression may be post-incremented. The C++ frontend's
`current_binding_level` macro similarly assumes an assignable conditional
expression. Modern GCC rejects both constructs. The
`bootstrap_jan25_gcc.sh` recipe applies semantics-preserving workarounds
*only in the temporary unpacked upstream tree*, not in tracked project files.

## Reproduction

Run on Debian x86_64 with `gcc`, `make`, `flex`, `bison`,
`curl`, `dpkg-deb`, `binutils`, `sha256sum`, and 32-bit glibc:

```sh
limactl start revcl-oracle
limactl shell revcl-oracle -- sh \
  /path/to/revcl/vuc/scripts/bootstrap_jan25_gcc.sh \
  /tmp/vuc-gcc333-jan25 2
```

The script pins downloads by SHA-256, configures a compiler-only
`i686-linux-gnu` cross toolchain and builds it without altering VCL.
GCC 3.3.3's old C++ frontend and i386 code generator **built successfully**
in the existing Debian 12 x86_64 Lima VM. The resulting
`/tmp/vuc-gcc333-jan25/build/gcc/xgcc --version` prints
`xgcc (GCC) 3.3.3 20040125 (prerelease)`. It is a modern-host cross compiler
using that historical compiler source, not the original Debian binary.

The `all-gcc` runtime phase fails because its target startup/libgcc files
need a period-compatible i386 sysroot, including i386 development headers.
The reproducible recipe instead builds and verifies compiler-only targets
`xgcc` and `cc1plus`. It creates a runnable
`/tmp/vuc-gcc333-jan25/g++-jan25-candidate` wrapper with 32-bit assembler
mode and compatibility headers. Those C++ headers currently come from the
separately pinned Debian GCC **3.3.5** development package (2005), not the
unrecovered exact 2004 libstdc++ development package.

All six optimization configurations passed and generated real ELF32 i386
relocatable objects. The flags are inspected through
`vuc/scripts/compiler_probe.py`; its manifest correctly marks this compiler
as a version-matched **candidate**, not historically authenticated.

## Tested VUC function and compiler options

The original `AsmOutput_WritePrologue` is 30 bytes. Compiling the plausible
source in `vuc/probes/asm_output_hypothesis.cpp` with the recovered GCC
`-O2` yields 22/30 matching function bytes and 92/92 matching literal bytes,
after resolving two i386 relocations. GCC 3.3.3 emits `.rodata` rather than
GCC 3.3.5's `.rodata.str1.32`; the Jan-2004 relocation manifest is
`vuc/config/asm_output_jan25_probe.json`.

Run the reproducible flag sweep:

```sh
python3 /path/to/revcl/vuc/scripts/compare_variants.py \
    --compiler /tmp/vuc-gcc333-jan25/g++-jan25-candidate \
    --out /tmp/vuc-gcc333-jan25/variant-comparisons
```

The sweep includes O0/O1/O2/Os, register renaming, scheduling, GCSE,
peepholes, reordering, aliasing and several x86 architectures. None matched
all 30 bytes. Standard `-O2`, `-O2 -frename-registers`, and several related
variants matched 22/30. `-O2 -fno-schedule-insns2` matched 14/30. The
`-O2 -march=i386/i486/pentium` variants emitted bodies with different sizes,
whereas `-O2 -march=i686` preserved a 30-byte function. A rejected flag
(`-mtune=pentium3`) is recorded as a failure, not a comparison result.

**These experiments neither prove nor disprove exact Debian compiler
equivalence.** Source changes can affect the small register-ordering
differences, and we still lack Debian-specific patches, the 2004 C++ headers,
exact binutils, and period glibc. The next acceptance requirement is an
evidence-backed code-generation match on multiple independent functions,
followed by toolchain and linker provenance verification.

Do not equate a matching compiler version string with a proven complete
byte-exact build environment.

## Later finding: archived GNU as 2.15 restores exact padding

After recovering the original source-level hash loop, the historical GCC
frontend produced equivalent instructions but its output assembled by Debian
12 binutils 2.40 still differed in loop-alignment padding. An extracted,
SHA-256-pinned Debian GNU as 2.15 produces the original 86-byte hash-loop
body **exactly**, with its relocation resolved. The original VUC assembler
version is **not** authenticated; this is a suitable compatible toolchain
candidate for function-level validation, not the final ELF build environment.

For the reproducible version and complete accepted-source results, see
`vuc/scripts/bootstrap_binutils215.sh` and
`vuc/docs/historical-assembler-milestone.md`.

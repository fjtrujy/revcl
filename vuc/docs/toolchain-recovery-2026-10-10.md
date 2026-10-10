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
The original Debian patch set, exact binutils version, historical glibc,
and C++ runtime linkage remain unverified until corresponding artifacts
are recovered and code-generation probes are compared.

Do not equate a matching compiler version string with a proven complete
byte-exact build environment.

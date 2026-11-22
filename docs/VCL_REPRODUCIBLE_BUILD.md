# VCL 1.4 beta 7: native byte-identical reconstruction

## Result (22 November 2026)

The C++ reconstruction of the original Linux x86 Sony VCL 1.4 beta 7
executable in `src/` has reached a **100% byte match**. The result has been
verified not only by the relocation-aware reconstruction/comparison tool, but
by hashing the **actual output of the historical GNU linker and `strip`**.
No binary postprocessing, original-byte copying, or `.dynsym` rewriting is
needed for the native linked executable.

| Evidence | Result |
| --- | --- |
| Archived target | `archive/upstream/vcl/downloads/vcl.1.4.beta7.x86/vcl_14beta7_x86` |
| ELF format | 32-bit x86, dynamically linked, stripped |
| File size | 482,736 bytes |
| Functions | 1,192/1,192 byte-exact; 268,717/268,717 code bytes |
| Inter-function padding | 1,975 verified bytes |
| Data | 151,250 matched bytes |
| Exception/unwind tables | 48,741 matched bytes |
| Linker/runtime bytes | 12,053 matched bytes |
| Total score | **482,736/482,736 bytes (100%)** |
| Reconstructed output | `tmp/build/rebuilt.bin`: exact target SHA-256 |
| Actual linked output | `tmp/build/link/vcl.stripped`: exact target SHA-256 |

Expected SHA-256 for **all three** ELF files (archived original,
reconstructed output and native linked output):

```text
83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274
```

The initial complete-score milestone had a non-identical native linker output:
although the project reconstructed 482,736 correct bytes, the directly linked
ELF differed by 21 bytes. Those differences were exclusively `st_size` fields
of imported, undefined `STT_FUNC` entries in the ELF `.dynsym` table.
The root cause was **which historical glibc binary the linker used**.

We identified the exact historical library variant from the original dynamic
symbol sizes. The base Red Hat 9 `glibc-2.3.2-11.9.i386` matched only 8/26
imported libc function sizes, while `glibc-2.3.2-11.9.i686` matched 25/26.
The original Red Hat 9 update **`glibc-2.3.2-27.9.7.i686`** matched **26/26**.
With that library in the Docker root filesystem, recompiling and linking
produced the original SHA-256 directly.

## Toolchain and reproducibility

The toolchain intentionally reproduces its period rather than relying on the
host's compiler or glibc:

| Component | Pinned version |
| --- | --- |
| Operating system | Red Hat Linux 9 i386 userland in a `linux/386` Docker container |
| C/C++ compiler | GCC 3.2.2 20030222 (Red Hat Linux 3.2.2-5) |
| Assembler/linker | GNU binutils 2.13.90.0.18-9 |
| Compiler settings | `-O2 -frename-registers` (see `config/build.json`) |
| Base libraries | Red Hat 9 packages, including glibc 2.3.2-11.9 |
| Native-link glibc | **2.3.2-27.9.7.i686**, overlaid *last* |
| RPM authenticity | Every RPM pinned by `tools/rh9/rpms.sha256` |

The i686 glibc RPM checksum is:

```text
49e001e945f2a50626dbe0b9faab7e76a0c2a9e76bc136a4e95a9140142baecb
```

The bootstrap downloads historical RPM packages over HTTPS, checks every
SHA-256 before extraction, then overlays the i686 glibc update last. The
`tools/rh9/rootfs/` and `tools/rh9/rpms/` directories remain untracked;
only the Dockerfile, RPM checksum manifest and bootstrap script are committed.
The original reference executable is read-only and remains unmodified.

### Reproduce on Ubuntu Linux

The build needs Python 3.11 or compatible, Docker with `linux/386` execution
support, `curl`, `sha256sum`, `rpm2cpio`, and `cpio`:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends rpm2cpio cpio curl ca-certificates

bash scripts/ci/bootstrap_rh9.sh
docker build --platform linux/386 -t revcl-rh9 tools/rh9

python3 -m venv .venv
.venv/bin/pip install -r scripts/requirements.txt
.venv/bin/python -m unittest discover -s tests/unit -q
.venv/bin/python scripts/score.py --quiet
.venv/bin/python scripts/ci/verify_vcl_sha.py
```

The final step verifies the independently measured score, checks for build or
symbol-resolution errors, and calculates SHA-256 for the original ELF,
`rebuilt.bin`, and **the unmodified native stripped ELF**. Every digest
must equal the pinned target above. Any difference is a hard failure.

The link process preserves the original object order and uses the historical
GNU linker with an original-address-aware linker script (see `scripts/link.py`).
The linker script pins section addresses to faithfully reproduce the original
layout; **the generated native ELF itself is not patched**. The
relocation-aware `rebuilt.bin` is a separate diagnostic and independently
verified output, not a substitute for checking the native link.

## GitHub Actions

[`.github/workflows/vcl-byte-exact.yml`](../.github/workflows/vcl-byte-exact.yml)
runs the full build on pushes to `main` and `ci/**`, on pull requests, and
via manual dispatch. The job:

1. Checks out the full source and archived reference.
2. Prepares Python and extracts the checksum-verified RH9 toolchain RPMs.
3. Builds the period-correct Docker image.
4. Runs unit tests, builds all source files and produces the native link.
5. Requires the same SHA-256 for both generated ELFs and the original.

On failure, build, link and comparison logs are uploaded as an artifact.
The RPM download cache is keyed to the SHA-256 manifest; cached RPMs are
**still checked before extraction**.

The separate [modern compiler workflow](../.github/workflows/vcl-modern.yml)
builds and links the full 131-source reconstruction using both GCC 13 and
Clang 18 with a 32-bit Linux target and C++98 language mode, runs the Python
unit tests, and tests the two resulting executables against the eleven
self-contained VCL fixture cases. The 12th historical oracle case
`archived_basic` additionally requires the external MASP preprocessor,
so it remains in the dedicated [oracle instructions](../reverse/oracle/README.md).
The browser-based Assembly Workbench integration tests also require their
own historical compiler/browser environment. Neither is falsely reported
as run by the modern compiler workflow.

In modern Clang, the original GCC 3.2-only `#pragma implementation`
used for deferred inline `RegWeb` functions is not supported; the modern
Clang build makes the existing inline header visible to the `main.cpp`
translation unit. This does not modify the byte-exact historical build.
The original source's multiline diagnostic string literals are normalized
to `\n` escapes, a change independently verified to preserve the original
ELF SHA-256.

The local end-to-end proof compiled **131 source units**, matched all 1,192
functions, and passed **109 unit tests**. Negative checks confirmed that
changing one byte in either output, including a former `.dynsym` difference,
causes the verifier to fail. An independent GitHub Actions run is the final
check that a clean hosted runner reproduces the same result.

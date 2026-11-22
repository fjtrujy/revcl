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
| Standard native Makefile output | `tmp/native/vcl.stripped`: exact target SHA-256 |
| Decompilation diagnostic linked output | `tmp/build/link/vcl.stripped`: exact target SHA-256 |

Expected SHA-256 for the archived original and every native/diagnostic
ELF output listed above:

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
| Compiler settings | `-O2 -frename-registers` plus 20 owner-specific flags (see `Makefile`) |
| Build orchestrator | GNU Make 3.79.1 in the RH9 container |
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

The build requires Docker with `linux/386` execution support, `curl`,
`sha256sum`, `rpm2cpio`, and `cpio`. No Python package is required to build
the original SHA-identical VCL:

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends rpm2cpio cpio curl ca-certificates

bash scripts/ci/bootstrap_rh9.sh
docker build --platform linux/386 -t revcl-rh9 tools/rh9

docker run --rm --platform linux/386 \
  --mount type=bind,src="$PWD",dst=/work -w /work \
  revcl-rh9 make -j2
make verify-sha
```

The Docker `make` command compiles 131 translation units, links `tmp/native/vcl`
using the original GNU linker, and strips it to
`tmp/native/vcl.stripped`. Verification runs **on the host** since the
Red Hat 9 toolchain predates `sha256sum`. The Makefile supports GNU
`sha256sum` on Linux or `shasum -a 256` on macOS.

The build inputs are fixed and version-controlled:

- `Makefile`: original C++ compile flags, including 20 recovered per-file
  overrides, and ordinary compile/link/strip rules
- `config/vcl-link-order.mk`: exactly 131 source files in original link order
- `tools/rh9/vcl.ld`: original-address-aware linker script derived once
  from the historical GNU ld script
- `tools/rh9/vcl.sha256`: expected SHA-256 of the native stripped ELF
- `tools/rh9/rpms.sha256`: authenticated RPMs, including Red Hat 9 GNU
  Make 3.79.1

No original binary is read or patched in this build, and no dynamically
generated `PROVIDE()` symbols are necessary now that the implementation is
complete. The recovered section addresses and object order are nevertheless
essential to reproducing the exact historical bytes.

The older `scripts/score.py`, `scripts/build.py`, and `scripts/link.py`
are retained for reverse-engineering analysis and the Assembly Workbench.
They compare object files and generate a diagnostic reconstruction
(`tmp/build/rebuilt.bin`), but are **not part of the standard compilation or
historical SHA-256 CI**.

## GitHub Actions

[`.github/workflows/vcl-byte-exact.yml`](../.github/workflows/vcl-byte-exact.yml)
runs the full build on pushes to `main` and `ci/**`, on pull requests, and
via manual dispatch. The job:

1. Checks out the C++ source, Makefile, linker script and SHA checksum.
2. Downloads and verifies the RH9 toolchain RPMs.
3. Builds the period-correct Docker image.
4. Runs `make -j2` inside the original GCC/GNU Make container.
5. Runs `make verify-sha` on the modern Linux CI host to check the stripped
   native executable against the original SHA-256.

On failure, the generated native ELF files are uploaded as artifacts.
The RPM download cache is keyed to the SHA-256 manifest; cached RPMs are
**still checked before extraction**.

The separate [modern compiler workflow](../.github/workflows/vcl-modern.yml)
builds and links the full 131-source reconstruction using both GCC 13 and
Clang 18 with a 32-bit Linux target and C++98 language mode, runs the Python
unit tests (including native Makefile order/flag consistency), and tests the two resulting executables against the eleven
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

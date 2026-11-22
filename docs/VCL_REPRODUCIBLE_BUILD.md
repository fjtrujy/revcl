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
| Standard historical CMake output | `tmp/cmake/historical/vcl.stripped`: exact target SHA-256 |
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
| Compiler settings | `-O2 -frename-registers` plus 20 owner-specific flags from `config/build.json` |
| Build orchestrator | CMake 3.23+ on the modern host, with GCC 3.2.2 invoked in Docker |
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

### Reproduce with CMake

The historical build requires Docker with `linux/386` execution support,
CMake 3.23+, `curl`, `sha256sum`, `rpm2cpio`, and `cpio`. CMake drives
the historical GCC using a Docker compiler wrapper; no Python is needed
to produce the SHA-identical executable.

```sh
sudo apt-get update
sudo apt-get install -y --no-install-recommends cmake rpm2cpio cpio curl ca-certificates

bash scripts/ci/bootstrap_rh9.sh
docker build --platform linux/386 -t revcl-rh9 tools/rh9

cmake --preset historical
cmake --build --preset historical
ctest --preset historical
```

CMake compiles 131 translation units using the original GCC 3.2.2, links
`tmp/cmake/historical/vcl` with GNU ld, then uses the RH9 `strip` to
produce `tmp/cmake/historical/vcl.stripped`. CTest hashes that stripped
native ELF through CMake's built-in `file(SHA256)`, so the historical
container does not need a modern SHA utility. The CMake configure/build/test
commands also work on macOS when Docker Desktop is running and the RPM
bootstrap has been performed in a Linux environment.

The build inputs are fixed and version-controlled:

- `CMakeLists.txt`: native executable target, compile/link/strip rules,
  CTest registrations, and historical vs modern build modes
- `CMakePresets.json`: `historical`, `gcc`, and `clang` configurations
- `config/build.json`: recovered flags, including 20 owner-specific overrides;
  consumed directly by CMake for the historical build
- `config/vcl-link-order.cmake`: exactly 131 source files in original link
  order; CMake preserves that order when linking
- `tools/rh9/vcl.ld`: original-address-aware linker script derived once
  from the historical GNU ld script
- `tools/rh9/vcl.sha256`: expected SHA-256 of the native stripped ELF
- `tools/rh9/rpms.sha256`: authenticated original toolchain/library RPMs
- `cmake/toolchains/rh9.cmake` and `scripts/cmake/`: toolchain/compiler/strip
  wrappers that execute the untouched 2003 tools inside the pinned Docker image

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

1. Checks out the C++ source, CMake files, linker script and SHA checksum.
2. Downloads and verifies the RH9 toolchain RPMs.
3. Builds the period-correct Docker image.
4. Runs `cmake --preset historical` and `cmake --build --preset historical`.
   Each compiler/linker/strip invocation executes inside the original Docker
   toolchain; CMake itself runs on the modern host.
5. Runs `ctest --preset historical` to verify the native executable's SHA-256.

On failure, the generated native ELF files are uploaded as artifacts.
The RPM download cache is keyed to the SHA-256 manifest; cached RPMs are
**still checked before extraction**.

The separate [modern compiler workflow](../.github/workflows/vcl-modern.yml)
builds and links the full 131-source reconstruction using both GCC 13 and
Clang 18 with a 32-bit Linux target and C++98 language mode, runs the Python
unit tests (including CMake link-order/flag consistency), and runs all eleven
self-contained VCL fixture cases **as individually reported CTest tests**.
Its Ubuntu Linux setup uses `cmake --preset gcc` or `cmake --preset clang`,
followed by `cmake --build --preset ...` and `ctest --preset ...`.
The same workflow also builds native **macOS arm64 and x86_64 Mach-O**
executables using Apple Clang and `cmake --preset macos`, with identical
unit/fixture coverage. These native 64-bit binaries are functionally
tested, but are not expected to match the SHA of Sony's 32-bit Linux ELF.
The eleven standalone fixture outputs are verified against frozen SHA-256
hashes of the generated VU text in
`tests/oracle/expected_stdout_sha256.json`, so the same input must produce
identical text and exit status on Linux and both macOS architectures.
The legacy pointer-sized values and the i386 `Block`/`workTable` overlay
are adapted for the 64-bit macOS ABI with compile-time offset checks;
the historical i386 layout and original compiler output remain unchanged.

Local native macOS setup (with CMake 3.23+ and the Xcode Command Line Tools):

```sh
python3 -m venv .venv
.venv/bin/pip install -r scripts/requirements.txt
cmake --preset macos -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset macos
ctest --preset macos
```
The 12th historical oracle case
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

The CMake historical build has independently reproduced the exact original
SHA-256 from all 131 translation units. The modern CMake builds run the
separate unit and functional test suites. The old scorer is retained for
historical reverse-engineering diagnostics and is not invoked by either
new CMake workflow.

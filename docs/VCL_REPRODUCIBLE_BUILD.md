# Reproducing the original VCL 1.4 beta 7 binary

REVCL's reconstructed sources can be compiled using the historical Red Hat
Linux 9 GCC 3.2.2-5 compiler and i686 glibc update to reproduce the exact
482,736-byte Linux i386 VCL ELF. This repository includes **no original Sony
binary**. Only its expected SHA-256 is recorded for verification.

## Build inputs

- `src/`: 131 reconstructed translation units and associated headers.
- `config/vcl-link-order.cmake`: validated original object/link order;
  this determines link input order, not the order of parallel compilation.
- `config/build.json`: common historical C++ compiler flags and a compact
  mapping of the five per-unit extra flags still required for an exact match.
- The pinned RH9 toolchain's **default linker script** is sufficient; the
  previously included custom linker script was proven unnecessary.
- Common `-O2 -frename-registers -fmerge-all-constants` options reproduce
  the original ELF. The last option was promoted from five per-file entries
  after a full historical rebuild; five other per-file options remain.
- `tools/rh9/rpms.sha256`: pinned original Red Hat toolchain and library RPMs.
- `tools/rh9/Dockerfile`: a Docker environment built from those verified RPMs.
- `scripts/cmake/`: wrappers that invoke the historical compiler and strip
  tools inside Docker.
- `tools/rh9/vcl.sha256`: the expected reference SHA-256, not an executable.

## Reproduction

On Linux with Docker and CMake >= 3.23:

```sh
sudo apt-get install -y cmake rpm2cpio cpio curl ca-certificates
bash scripts/ci/bootstrap_rh9.sh
docker build --platform linux/386 -t revcl-rh9 tools/rh9
cmake --preset historical
cmake --build --preset historical --parallel "$(sh scripts/ci/cpu-count.sh)"
ctest --preset historical --output-on-failure
```

The stripped output lives at `tmp/cmake/historical/vcl.stripped` and must match:

```text
83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274
```

CTest runs `cmake/VerifySha.cmake` and checks the linked and stripped ELF
without patching or replacing any bytes. The original ELF is neither bundled
nor required during the build. The glibc i686 update matters: the original
Red Hat base i386 library leads to mismatched imported-symbol sizes, whereas
the verified i686 update reproduces them.

The [magic number and historical build audit](MAGIC_NUMBERS_AND_BUILD_AUDIT.md)
documents the experiments behind these simplifications, including why object
filenames do not determine link order and which historical per-unit flags
still change executable bytes.
The [enum-type and compiler-flag follow-up](ENUM_TYPES_AND_COMPILER_FLAGS.md)
documents the additional global-flag consolidation and recovered enum-typed
fields without changing the i386 ABI.

The historical process is independently repeated by
[GitHub Actions](../.github/workflows/vcl-byte-exact.yml).

## Native modern builds

Use `linux-gcc`, `linux-clang` or `macos` CMake presets. The modern builds
use the native 64-bit ABI and a shared C++98-compatible source tree. Their
output is verified for functional behavior with 11 standalone fixture hashes
and independent checks of the command-line interface and project metadata.
They are **not** expected to be byte-for-byte identical to a 32-bit Linux ELF.

These builds do not install Python, Docker, or the original compiler.
The [modern CI matrix](../.github/workflows/vcl-modern.yml) covers Linux
x86_64/ARM64 GCC and Clang and macOS Intel/Apple Silicon Apple Clang.

## Copyright and licensing

The historical toolchain RPMs retain their respective third-party licenses.
No Sony executable, manual, disassembly, or other third-party release archive
is stored here. The project's MIT license does not grant rights in Sony's
software or other third-party materials.

# REVCL

REVCL reconstructs Sony's historical PlayStation 2 **VCL 1.4 beta 7** Vector
Unit optimizer/compiler as portable C++ source. The reconstruction reproduces
the SHA-256 of the original stripped Linux i386 executable when built with the
period-correct Red Hat Linux 9 compiler and libraries.

The same C++ source builds natively on modern **Linux x86_64 / ARM64** and
**macOS Intel / Apple Silicon**. Modern binaries are checked with standalone
input fixtures, including exact generated-output hashes.

## Provenance and third-party rights

REVCL is an independent reverse-engineering project. It is not affiliated with,
endorsed by, or sponsored by Sony Interactive Entertainment. Sony, PlayStation,
and related marks belong to their respective owners.

**This repository does not include Sony's original executable, user manual,
archived third-party tool distributions, or the original binary's disassembly.**
The checksum below is a reference value, not a copy of the software. The MIT
license in this repository does not grant rights in Sony's original software,
its documentation, or other third-party materials.

## Build and test

CMake 3.23+ and a native 64-bit C++ compiler are required. Python is not needed.

### Linux (x86_64 or ARM64)

```sh
cmake --preset linux-gcc
cmake --build --preset linux-gcc --parallel "$(sh scripts/ci/cpu-count.sh)"
ctest --preset linux-gcc --parallel "$(sh scripts/ci/cpu-count.sh)" --output-on-failure
```

Use `linux-clang` instead of `linux-gcc` to compile with Clang.

### macOS (Apple Silicon or Intel)

```sh
cmake --preset macos
cmake --build --preset macos --parallel "$(sh scripts/ci/cpu-count.sh)"
ctest --preset macos --parallel "$(sh scripts/ci/cpu-count.sh)" --output-on-failure
```

To use the default native toolchain without a preset:

```sh
cmake -S . -B build
cmake --build build --parallel "$(sh scripts/ci/cpu-count.sh)"
ctest --test-dir build --parallel "$(sh scripts/ci/cpu-count.sh)" --output-on-failure
```

CTest verifies the CLI, project consistency and eleven complete-output fixture
hashes. All six native compiler/architecture combinations are also tested in
[GitHub Actions](.github/workflows/vcl-modern.yml).

## Reproduce the historical Linux x86 executable

The historical compiler and libraries are independently fetched, verified
against pinned hashes, and assembled into a Docker image. Neither the original
Sony executable nor its disassembly is used as a build input.

On a Linux development machine with Docker:

```sh
sudo apt-get install -y cmake rpm2cpio cpio curl ca-certificates
bash scripts/ci/bootstrap_rh9.sh
docker build --platform linux/386 -t revcl-rh9 tools/rh9
cmake --preset historical
cmake --build --preset historical --parallel "$(sh scripts/ci/cpu-count.sh)"
ctest --preset historical --output-on-failure
```

The stripped output must have this exact SHA-256:

```text
83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274
```

The original linked and stripped ELF is 482,736 bytes. Reproducibility is
verified directly by `cmake/VerifySha.cmake`; the SHA value is stored in
`tools/rh9/vcl.sha256`. No post-link binary patching is performed. See
[historical reproducible build details](docs/VCL_REPRODUCIBLE_BUILD.md).

## Development

The 131 C++ translation units are in `src/`. `config/vcl-link-order.cmake`
preserves their original object ordering and `config/build.json` preserves the
historical compiler options. The frozen fixtures are in `tests/oracle/`.
Reverse-engineering research material and third-party downloads are maintained
separately rather than redistributed here.

## Contributing and licensing

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for upstream provenance,
what is excluded from this repository, and the limits of its MIT license.
[CONTRIBUTING.md](CONTRIBUTING.md) describes how to submit code and tests
without adding proprietary or unreviewed reference materials.

The distinct VUC 2004.1 reconstruction is developed on the
[`vuc` branch](https://github.com/fjtrujy/revcl/tree/vuc).

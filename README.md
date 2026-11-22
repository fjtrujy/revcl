# REVCL

Modern reconstruction of Sony's historical PlayStation 2 VCL (Vector Unit
Command Line) optimizer/compiler tool.

The repository intentionally separates archived upstream artifacts from the
clean implementation:

- `archive/upstream/` preserves VCL, VUC, and VUEMU historical material.
- `docs/LEGACY_INVENTORY.md` records the initial technical analysis.
- `docs/REVCL_PLAN.md` is the phased reconstruction and validation plan.
- `tools/fetch_legacy_sources.py` reproducibly refreshes the archive.

Historical third-party files retain their original licenses and are not
relicensed by this repository's MIT license.

**VCL 1.4 beta 7 is now completely reconstructed byte-for-byte**: the C++
sources reproduce the original Sony Linux x86 executable, including the exact
SHA-256 of the native linked and stripped ELF. See
[`docs/VCL_REPRODUCIBLE_BUILD.md`](docs/VCL_REPRODUCIBLE_BUILD.md) for the
historical toolchain, original-library discovery, and reproducibility proof.
The separate [historical oracle](reverse/oracle/README.md) remains available
for behavioral testing.

## Matching decompilation of VCL 1.4 beta 7 (Linux x86)

A byte-for-byte matching C++ decompilation of
`archive/upstream/vcl/downloads/vcl.1.4.beta7.x86/vcl_14beta7_x86`
(GCC 3.2.2 Red Hat Linux 9, `-O2 -frename-registers`) lives in `src/`.
The source was reconstructed without embedding or modifying the original
executable in the build.

Progress (matched bytes / file size):

<!-- progress:start -->
**482,736 / 482,736 bytes (100.00%)** of `vcl_14beta7_x86` matched — code 268,717 / 268,717 (1192 / 1192 functions exact, 268,717 bytes), verified inter-function padding 1,975 bytes, data 151,250 bytes, unwind tables 48,741 bytes, linker/runtime 12,053 bytes
<!-- progress:end -->

### Build with CMake

The repository uses one CMake project with six presets: `historical`
(GCC 3.2.2 in Docker), `gcc` and `clang` (modern Linux i386),
`linux-gcc` and `linux-clang` (native Linux x86_64 or ARM64), and
`macos` (native arm64 or Intel x86_64).
CMake 3.23+ is required. For the byte-exact historical build,
first download the original, SHA-pinned Red Hat 9 compiler and libraries
(Linux, or Ubuntu in a container for the extraction step on macOS):

```sh
sudo apt-get install -y cmake rpm2cpio cpio curl ca-certificates
bash scripts/ci/bootstrap_rh9.sh       # download + SHA-verify historical RPMs
docker build --platform linux/386 -t revcl-rh9 tools/rh9
cmake --preset historical
cmake --build --preset historical
ctest --preset historical              # exact SHA-256 verification
```

For modern **native 64-bit Linux** (x86_64 or ARM64/aarch64), install
`cmake`, `g++`, `clang`, `python3-venv`, and `python3-pip`, then:

```sh
python3 -m venv .venv
.venv/bin/pip install -r scripts/requirements.txt

cmake --preset linux-gcc -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset linux-gcc
ctest --preset linux-gcc               # Python unit tests + 11 VCL fixtures

cmake --preset linux-clang -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset linux-clang
ctest --preset linux-clang
```

The original modern **32-bit Linux** compatibility targets remain available
on x86_64 machines with multilib packages installed:

```sh
cmake --preset gcc -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset gcc
ctest --preset gcc

cmake --preset clang -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset clang
ctest --preset clang
```

For **native macOS** (no Docker required), install CMake and the Xcode
Command Line Tools, then install the Python test dependencies:

```sh
python3 -m venv .venv
.venv/bin/pip install -r scripts/requirements.txt

cmake --preset macos -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset macos
ctest --preset macos                    # Python unit tests + 11 VCL fixtures
./tmp/cmake/macos/vcl -h
```

The `macos` preset follows the machine architecture: Apple Silicon produces
a native arm64 Mach-O executable; Intel Macs produce x86_64 Mach-O. This
modern executable is **functionally tested**, not SHA-identical to the
original 32-bit Linux ELF (which remains guaranteed by `historical`).
For all eleven standalone oracle inputs, CTest additionally checks the
complete generated output against SHA-256 baselines recorded in
`tests/oracle/expected_stdout_sha256.json`, so all modern Linux/macOS builds
must produce identical VU text, not merely exit successfully.

The **same C++ sources** are built on every platform, without macOS- or
Linux-specific execution paths. `intptr_t`/`uintptr_t` hold pointers and
pointer-derived handles on 32-bit and 64-bit systems; the original `Block`
interface inherits the real `workTable` layout instead of maintaining a
second platform-dependent overlay. Only the `historical` preset uses the
original compiler flags and toolchain. The native modern binaries are
functional equivalents, **not** expected to match the i386 ELF's SHA.

### Byte-exact CI

The [VCL byte-exact build](.github/workflows/vcl-byte-exact.yml) runs on
pushes to `main` or `ci/**`, pull requests and manual dispatches. A fresh Linux runner
downloads the original Red Hat Linux 9 GCC 3.2.2-5 toolchain and the
**glibc 2.3.2-27.9.7 i686** update, verifies each RPM against the pinned
SHA-256 manifest in `tools/rh9/rpms.sha256`, builds the historic Docker image,
then uses **CMake** to compile, link, and strip the full source tree using
the original GCC inside Docker. The recovered original object order lives in
`config/vcl-link-order.cmake`, compiler flags in `config/build.json`, and
the original GNU ld script in `tools/rh9/vcl.ld`.
The CI check runs the `exact-sha256` CTest, requiring the
**native linked and stripped ELF** (`tmp/cmake/historical/vcl.stripped`) to match the
archived ELF's **exact SHA-256**:
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.
Neither `scripts/score.py` nor the archived original ELF is used as a build
input. `scripts/score.py` remains available for decompilation diagnostics,
including per-function byte matching, coverage, and a separate
`tmp/build/rebuilt.bin`. Tests run independently in the modern compiler CI.

The glibc i686 update is essential: using the i386 glibc from the base
Red Hat 9 RPMs produces 21 different `.dynsym` bytes (external imported
function sizes). The updated i686 library recovers all of those sizes in
the native link itself; **no binary patching or symbol rewriting is used**.
The full reproducibility report and verification procedure are in
[`docs/VCL_REPRODUCIBLE_BUILD.md`](docs/VCL_REPRODUCIBLE_BUILD.md).

### Developer tools

The independent [modern compiler workflow](.github/workflows/vcl-modern.yml)
builds all 131 C++ translation units with modern GCC and Clang for 32-bit
Linux i386, native 64-bit Linux x86_64/ARM64, and with Apple Clang for
macOS arm64/Intel x86_64. For the private repository, Linux ARM64 CI uses
an emulated ARM64 Linux userland on a standard GitHub runner. It
produces runnable binaries and uses CTest to run the Python unit suite and
all 11 standalone oracle fixture cases independently (including both
expected invalid inputs). It does
**not** require the historical toolchain and intentionally does not require
a SHA match from the modern compilers. Modern Linux binaries are available
as GitHub Actions artifacts.

Two source files contain historical diagnostic messages that used physical
newlines inside quoted strings. They have been normalized to standard
`\n` escapes to satisfy modern compilers; verification with GCC 3.2.2
confirmed this does not change the original SHA-256.

For optional reverse-engineering diagnostics, install the Python tooling
separately; it is **not needed** for the historical CMake build:

```sh
python3 -m venv .venv
.venv/bin/pip install -r scripts/requirements.txt
scripts/decomp status                  # full score and coverage
scripts/decomp diff 0x08058dd8         # compare a function with the original
scripts/decomp lab 0x08080460          # editable source / original ASM workbench
scripts/decomp score --html            # regenerate tmp/progress.html
```

Open `tmp/progress.html` for the visual comparison of the original and the
rebuilt bytes and the progress history (`progress/history.csv`); its
`#run` tab ("Agent run") follows a running `iterate.py` live. Details: [`docs/decompilation.md`](docs/decompilation.md)
(approach, toolchain, scoring), [`docs/workflow.md`](docs/workflow.md) (agent
loop), [`docs/matching-notes.md`](docs/matching-notes.md) (compiler idioms),
[`AGENTS.md`](AGENTS.md) (rules for agents).

### Interactive source ↔ assembly experiments

The **[Assembly Workbench](docs/ASM_LAB.md)** is a permanent local browser
tool modeled on a three-pane decompilation editor: editable, syntax-highlighted
C/C++ source, relocation-resolved generated i386 assembly, and original
VCL assembly. Matching instructions are green; mismatches are red; missing
or extra instructions are amber. It offers a source-file explorer, synchronized
source/ASM scrolling, bidirectional source caret ↔ instruction highlighting
using code-verified historical GCC DWARF source lines, mismatch navigation,
quick original-GCC compilation,
compiler diagnostics, and a separate fixed-offset raw-byte score.

Run `scripts/decomp lab 0x08081610` to start experimenting with
`findInsertPositionMode0`, or pass any mapped original function address.
**Changes are saved only under ignored `tmp/asm_lab/`**, never automatically
into `src/`. The project comparison and full regression gates remain
authoritative for accepting a source change.

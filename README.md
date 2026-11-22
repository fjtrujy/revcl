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

Build and verify on Linux (or in an Ubuntu environment with Docker):

```sh
sudo apt-get install -y rpm2cpio cpio curl ca-certificates
bash scripts/ci/bootstrap_rh9.sh       # download + SHA-verify historical RPMs
docker build --platform linux/386 -t revcl-rh9 tools/rh9
python3 -m venv .venv
.venv/bin/pip install -r scripts/requirements.txt
.venv/bin/python scripts/score.py --quiet
.venv/bin/python scripts/ci/verify_vcl_sha.py
```

### Byte-exact CI

The [VCL byte-exact build](.github/workflows/vcl-byte-exact.yml) runs on
pushes to `main` or `ci/**`, pull requests and manual dispatches. A fresh Linux runner
downloads the original Red Hat Linux 9 GCC 3.2.2-5 toolchain and the
**glibc 2.3.2-27.9.7 i686** update, verifies each RPM against the pinned
SHA-256 manifest in `tools/rh9/rpms.sha256`, builds the historic Docker image,
then recompiles and links the entire VCL source tree. It also runs the VCL
unit tests.
The CI check runs `scripts/ci/verify_vcl_sha.py`, requiring **both** the
native linked and stripped ELF (`tmp/build/link/vcl.stripped`) and the
reconstructed output (`tmp/build/rebuilt.bin`) to match the archived ELF's
**exact SHA-256**:
`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`.
It also requires 1,192/1,192 exact functions, full byte coverage and
zero build or relocation errors.

The glibc i686 update is essential: using the i386 glibc from the base
Red Hat 9 RPMs produces 21 different `.dynsym` bytes (external imported
function sizes). The updated i686 library recovers all of those sizes in
the native link itself; **no binary patching or symbol rewriting is used**.
The full reproducibility report and verification procedure are in
[`docs/VCL_REPRODUCIBLE_BUILD.md`](docs/VCL_REPRODUCIBLE_BUILD.md).

### Developer tools

After completing the setup above:

```sh
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

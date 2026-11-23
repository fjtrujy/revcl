# Automated REVCL releases

REVCL uses a **tag-triggered GitHub Actions pipeline** rather than manually
uploading binaries. The pipeline in `.github/workflows/vcl-release.yml`
rebuilds, tests, packages and publishes releases from a single source commit.

The first release, [v1.0.0](https://github.com/fjtrujy/revcl/releases/tag/v1.0.0),
was created manually before this automation. The automated workflow is for
**v1.0.1 and later**.

## Releasing a new version

First merge changes into `main` and ensure the normal historical and native
CI workflows pass. Then run:

```sh
git switch main
git pull --ff-only origin main
git tag -a v1.0.1 -m "REVCL v1.0.1"
git push origin v1.0.1
```

Replace `v1.0.1` with the intended version. Tags must be **annotated**
and use the format `vMAJOR.MINOR.PATCH`, without prerelease suffixes.
The tag must target a commit reachable from remote `main`. Do not reuse
a published tag; create a new version.

The tag push triggers the
[VCL tagged release workflow](../.github/workflows/vcl-release.yml).
After the workflow succeeds, GitHub publishes the release at a URL such as
`https://github.com/fjtrujy/revcl/releases/tag/v1.0.1`.

### Dry runs (no publication)

Use GitHub Actions → **VCL tagged release** → **Run workflow**,
selecting `main` or another branch. This manually triggered run builds,
tests, verifies and packages the seven executables, then uploads the
`revcl-release-packages` Actions artifact, but **never creates or edits
a GitHub Release**. The same non-publishing path runs on PRs that change
release-related files.

## What is built and verified

Every build comes from the same Git commit:

| Release ZIP suffix | Platform | Compiler |
| --- | --- | --- |
| `linux-x86_64-gcc` | Linux x86-64 | GCC |
| `linux-x86_64-clang` | Linux x86-64 | Clang |
| `linux-aarch64-gcc` | Linux ARM64 | GCC |
| `linux-aarch64-clang` | Linux ARM64 | Clang |
| `macos-arm64-apple-clang` | macOS Apple Silicon | Apple Clang |
| `macos-x86_64-apple-clang` | macOS Intel | Apple Clang |
| `linux-i386-original-sha256` | Historical Linux i386 | Red Hat 9 GCC 3.2.2 |

The six modern jobs each run the full native CTest suite before making the
compiled binary available to packaging. The historical job builds from the
pinned compiler and checks both the original executable SHA and historical
compiler-option necessity. Any failing build, test or missing artifact
blocks publication.

The packaging job downloads **only verified artifacts from this workflow**,
checks file formats, and obtains the original Sony VCL x86 distribution.
It validates that archive using `tools/rh9/vcl-reference-archive.sha256`,
extracts the original VCL license, and directly compares the rebuilt
historical binary with Sony's original executable. The original vendor
executable is neither committed nor copied into the release; the bundled
historical executable was rebuilt from REVCL's C++ source and has the same
bytes.

Packaging is implemented in `scripts/ci/package-release.sh` and produces
seven ZIPs, `SHA256SUMS.txt` (release archive hashes),
`BINARY_SHA256SUMS.txt` (hashes of executables inside ZIPs),
`BUILD_PROVENANCE.txt`, and generated `RELEASE_NOTES.md`.
Each ZIP includes the executable, platform README, REVCL license, and
third-party notices. The historical ZIP additionally includes the original
Sony VCL license.

## Publishing and error handling

The publishing job runs **only for pushed version tags**, requires an
annotated tag reachable from `main`, and runs only after the other jobs
succeed. All preceding jobs have read-only repository permissions; the
publication job alone has `contents: write`.

The workflow creates a **draft GitHub release**, uploads the verified
assets, then publishes it. If publishing fails, the release stays a draft.
Re-running the workflow can finish that draft, while already published
releases are protected against accidental overwrite.

If the vendor reference cannot be downloaded or any checksum comparison
fails, publishing stops. It never silently omits the original-compatible
binary or the accompanying license. Resolve the cause and rerun the
workflow.

The historic i386 executable requires a compatible 32-bit Linux runtime.
Modern Linux binaries are dynamically linked. There are no Windows or
native PS2 executables in the current build matrix.

## Verify a release

Download `SHA256SUMS.txt` together with the release assets and run:

```sh
shasum -a 256 -c SHA256SUMS.txt
```

The historical i386 binary must match:

`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`

See [third-party notices](../THIRD_PARTY_NOTICES.md) for the rights
constraints on unmodified Sony VCL redistribution. The REVCL MIT source
license does not relicense Sony's original software.

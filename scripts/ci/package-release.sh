#!/usr/bin/env bash
# Package all verified REVCL binaries from ONE GitHub Actions run.
# Usage: package-release.sh VERSION NATIVE_ARTIFACTS_DIR HISTORICAL_ELF REFERENCE_TGZ OUTPUT_DIR
set -euo pipefail
if [ "$#" -ne 5 ]; then
  echo "usage: $0 vX.Y.Z native-artifacts-dir historical-vcl.stripped reference.tgz output-dir" >&2
  exit 2
fi
version="$1"
native_dir="$2"
historical_elf="$3"
reference_archive="$4"
output="$5"

if [[ ! "$version" =~ ^v(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)$ ]]; then
  echo "Release version must be vMAJOR.MINOR.PATCH (got: $version)" >&2
  exit 2
fi

root="$(cd "$(dirname "$0")/../.." && pwd -P)"
cd "$root"
for input in "$historical_elf" "$reference_archive"; do
  if [ ! -f "$input" ]; then
    echo "Missing required release input: $input" >&2
    exit 1
  fi
done
historical_elf="$(cd "$(dirname "$historical_elf")" && pwd -P)/$(basename "$historical_elf")"
reference_archive="$(cd "$(dirname "$reference_archive")" && pwd -P)/$(basename "$reference_archive")"
native_dir="$(cd "$native_dir" && pwd -P)"
mkdir -p "$output"
output="$(cd "$output" && pwd -P)"
if [ -n "$(find "$output" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
  echo "Release output directory must be empty: $output" >&2
  exit 1
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
bundle_root="$work/bundles"
mkdir -p "$bundle_root"
source_sha="$(git rev-parse HEAD)"
ref_sha="$(awk 'NR==1 {print $1}' tools/rh9/vcl.sha256)"
archive_sha="$(awk 'NR==1 {print $1}' tools/rh9/vcl-reference-archive.sha256)"

hash_file() { shasum -a 256 "$1" | awk '{print $1}'; }
verify_file() {
  local actual
  actual="$(hash_file "$1")"
  if [ "$actual" != "$2" ]; then
    echo "SHA-256 mismatch: $1 (got $actual, expected $2)" >&2
    exit 1
  fi
}
verify_file "$reference_archive" "$archive_sha"
verify_file "$historical_elf" "$ref_sha"
reference_binary="$work/original-vcl"
tar -xzOf "$reference_archive" vcl.1.4.beta7.x86/vcl_14beta7_x86 > "$reference_binary"
verify_file "$reference_binary" "$ref_sha"
if ! cmp -s "$historical_elf" "$reference_binary"; then
  echo "Historical rebuilt binary differs from original reference executable" >&2
  exit 1
fi
tar -xzOf "$reference_archive" vcl.1.4.beta7.x86/LICENSE > "$work/SONY-VCL-LICENSE.txt"
grep -q 'copy and redistribute the software and documentation' "$work/SONY-VCL-LICENSE.txt"

# Fixed archive timestamps and sorted entries keep packaging predictable.
new_bundle() {
  local name="$1" description="$2" provenance="$3"
  local dest="$bundle_root/$name"
  mkdir -p "$dest"
  cp LICENSE "$dest/REVCL-LICENSE.txt"
  cp THIRD_PARTY_NOTICES.md "$dest/THIRD_PARTY_NOTICES.md"
  cat > "$dest/README.txt" <<EOF
REVCL $version — $description

Source revision: $source_sha
Platform: $description
Build provenance: $provenance
Usage: ./vcl [options]

Linux binaries require their respective system dynamic linker/libraries.
The historical Linux i386 executable requires a compatible 32-bit runtime.
This is not a Windows or PlayStation 2 native executable.

See REVCL-LICENSE.txt and THIRD_PARTY_NOTICES.md.
EOF
  echo "$dest"
}
archive_bundle() {
  local name="$1"
  # Prevent filesystem timestamps from causing avoidable differences in ZIPs.
  find "$bundle_root/$name" -exec touch -t 202001010000 {} +
  (cd "$bundle_root"; find "$name" -print | LC_ALL=C sort | zip -X -q "$output/$name.zip" -@)
  unzip -t "$output/$name.zip" | grep -q 'No errors detected'
  printf '%s  %s/vcl\n' "$(hash_file "$bundle_root/$name/vcl")" "$name" >> "$output/BINARY_SHA256SUMS.txt"
}

# Tag source and compiler variants correspond exactly to vcl-modern.yml.
# Each source directory is the output of actions/download-artifact, not a
# file built on the packaging runner.
targets=(
  "linux-x86_64-gcc:revcl-release-native-linux-x86_64-gcc:ELF 64-bit.*x86-64"
  "linux-x86_64-clang:revcl-release-native-linux-x86_64-clang:ELF 64-bit.*x86-64"
  "linux-aarch64-gcc:revcl-release-native-linux-arm64-gcc:ELF 64-bit.*ARM aarch64"
  "linux-aarch64-clang:revcl-release-native-linux-arm64-clang:ELF 64-bit.*ARM aarch64"
  "macos-arm64-apple-clang:revcl-release-native-macos-arm64:Mach-O 64-bit.*arm64"
  "macos-x86_64-apple-clang:revcl-release-native-macos-x86_64:Mach-O 64-bit.*x86_64"
)
: > "$output/BINARY_SHA256SUMS.txt"
for entry in "${targets[@]}"; do
  IFS=: read -r platform artifact file_pattern <<< "$entry"
  binary="$native_dir/$artifact/vcl"
  if [ ! -s "$binary" ]; then
    echo "Missing or empty CI native binary: $binary" >&2
    exit 1
  fi
  if ! file -b "$binary" | grep -Eq "$file_pattern"; then
    echo "Wrong binary format for $platform: $(file -b "$binary")" >&2
    exit 1
  fi
  name="revcl-$version-$platform"
  bundle="$(new_bundle "$name" "$platform" "GitHub Actions native matrix")"
  cp "$binary" "$bundle/vcl"
  chmod 755 "$bundle/vcl"
  archive_bundle "$name"
  echo "Packaged $name"
done

historical_name="revcl-$version-linux-i386-original-sha256"
historical_bundle="$(new_bundle "$historical_name" "Linux i386 (original byte-exact build)" "Red Hat Linux 9 GCC 3.2.2, i686 glibc")"
cp "$historical_elf" "$historical_bundle/vcl"
chmod 755 "$historical_bundle/vcl"
cp "$work/SONY-VCL-LICENSE.txt" "$historical_bundle/SONY-VCL-LICENSE.txt"
cat >> "$historical_bundle/README.txt" <<EOF

The included vcl is byte-for-byte identical to Sony's VCL 1.4 beta 7
vcl_14beta7_x86 from the historical x86 distribution.
Expected SHA-256: $ref_sha
Expected length: 482736 bytes
The executable is a rebuild of REVCL's C++ source, not a copy of the vendor
archive. Sony's unmodified software redistribution terms accompany this
binary as SONY-VCL-LICENSE.txt. REVCL's MIT source license does not relicense
the original Sony software.
EOF
archive_bundle "$historical_name"

ci_reference="Not available (local packaging validation)"
if [ -n "${GITHUB_RUN_ID:-}" ] && [ -n "${GITHUB_REPOSITORY:-}" ]; then
  ci_reference="${GITHUB_SERVER_URL:-https://github.com}/${GITHUB_REPOSITORY}/actions/runs/${GITHUB_RUN_ID}"
fi
cat > "$output/BUILD_PROVENANCE.txt" <<EOF
REVCL $version
Source revision: $source_sha
Build-and-test workflow: $ci_reference
Reference archive:
https://downloads.sourceforge.net/project/kernelloader/Sony%20Linux%20Toolkit/Package%20Update%20Files/vcl/vcl.1.4.beta7.x86.tgz
Reference archive SHA-256: $archive_sha
Reference i386 executable SHA-256: $ref_sha
Original reference and historical rebuild were verified byte-for-byte equal.
All six modern binaries are outputs of the corresponding CI matrix jobs.
EOF

cat > "$output/RELEASE_NOTES.md" <<EOF
# REVCL $version — VCL 1.4 beta 7 reconstructed from C++

This release contains seven executables built from source revision
\`$source_sha\`. All builds are tested before publication.

## Binary downloads

| Archive | Platform | Build compiler |
| --- | --- | --- |
| \`revcl-$version-linux-x86_64-gcc.zip\` | Linux x86-64 | GCC |
| \`revcl-$version-linux-x86_64-clang.zip\` | Linux x86-64 | Clang |
| \`revcl-$version-linux-aarch64-gcc.zip\` | Linux ARM64 | GCC |
| \`revcl-$version-linux-aarch64-clang.zip\` | Linux ARM64 | Clang |
| \`revcl-$version-macos-arm64-apple-clang.zip\` | macOS Apple Silicon | Apple Clang |
| \`revcl-$version-macos-x86_64-apple-clang.zip\` | macOS Intel | Apple Clang |
| \`revcl-$version-linux-i386-original-sha256.zip\` | Historical Linux i386 (original-compatible) | Red Hat 9 GCC 3.2.2 |

Unpack the appropriate archive and run \`./vcl\`. Linux executables are
dynamically linked. The original-compatible Linux i386 executable requires
a compatible 32-bit Linux runtime. No Windows or PS2 native executable is
provided in this release.

## Byte-exact historical build

The historical Linux i386 executable is rebuilt from this repository's C++
source, not copied from the original Sony archive. The release pipeline
compares the rebuilt binary directly with Sony's archived VCL 1.4 beta 7
\`vcl_14beta7_x86\` before publication.

- **SHA-256:** \`$ref_sha\`
- **Size:** 482,736 bytes
- **Format:** stripped Linux i386 ELF

Sony's original VCL \`LICENSE\` accompanies the historical executable
inside that ZIP as \`SONY-VCL-LICENSE.txt\`. The license permits
redistribution of the **unmodified original software** with the license;
REVCL's MIT source license does not relicense Sony's material. See the
included third-party notices for limitations and provenance.

## Validation and integrity

The six modern compiler/architecture combinations pass their native CTest
suites. The historical build passes the full executable SHA-256 check and
the compiler-override-necessity test. Download \`SHA256SUMS.txt\` to verify
the release archives and \`BINARY_SHA256SUMS.txt\` for executable hashes;
\`BUILD_PROVENANCE.txt\` records the exact source revision and workflow.

REVCL is an independent reconstruction, not endorsed by or affiliated with
Sony. The separate VUC reconstruction is not part of this release.
EOF

# Record SHA-256 for downloadable archives and internal executables.
(cd "$output"; shasum -a 256 ./*.zip BINARY_SHA256SUMS.txt BUILD_PROVENANCE.txt RELEASE_NOTES.md > SHA256SUMS.txt)
echo "Packaged seven binaries in $output"

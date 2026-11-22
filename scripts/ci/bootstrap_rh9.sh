#!/usr/bin/env bash
# Reconstruct the exact Red Hat Linux 9 GCC 3.2.2-5 Docker build context.
# Run on Linux with curl, sha256sum, rpm2cpio and cpio installed.
# RPM payloads and the extracted rootfs stay untracked.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
cd "$root"

manifest="tools/rh9/rpms.sha256"
base_url="https://rpmfind.net/linux/redhat-archive/9/en/os/i386/RedHat/RPMS"
update_url="https://rpmfind.net/linux/redhat-archive/updates/9/en/os/i686"
test -s "$manifest"
mkdir -p tools/rh9/rpms

while read -r expected path; do
    [[ "$expected" =~ ^[0-9a-f]{64}$ ]] || {
        echo "Invalid RPM SHA-256 entry: $expected" >&2
        exit 1
    }
    [[ "$path" =~ ^tools/rh9/rpms/[A-Za-z0-9.+_-]+\.(i386|i686)\.rpm$ ]] || {
        echo "Invalid RPM path: $path" >&2
        exit 1
    }
    if [[ -f "$path" ]] && echo "$expected  $path" | sha256sum --check --status; then
        echo "Verified existing $path"
        continue
    fi
    rm -f "$path" "$path.part"
    echo "Fetching $path"
    url="$base_url/${path##*/}"
    if [[ "$path" == "tools/rh9/rpms/glibc-2.3.2-27.9.7.i686.rpm" ]]; then
        url="$update_url/${path##*/}"
    fi
    curl --fail --location --silent --show-error --retry 4 --retry-delay 2 \
        --output "$path.part" "$url"
    echo "$expected  $path.part" | sha256sum --check --status || {
        echo "Downloaded RPM checksum mismatch: $path" >&2
        exit 1
    }
    mv "$path.part" "$path"
done < "$manifest"

sha256sum --check "$manifest"
rm -rf tools/rh9/rootfs
mkdir -p tools/rh9/rootfs

# The manifest's package order determines which RPM owns overlapping
# historical files. In particular, the RH9 i686 glibc security update MUST
# be last: its libc.so.6 exports give the original linker's imported STT_FUNC
# st_size words and restore exact native ELF SHA-256 without postprocessing.
# Extract each only after verifying its full RPM SHA-256.
while read -r _ path; do
    echo "Extracting $path"
    rpm2cpio "$path" | (cd tools/rh9/rootfs && cpio --extract --make-directories \
        --unconditional --quiet)
done < "$manifest"

mkdir -p tools/rh9/rootfs/tmp tools/rh9/rootfs/var/tmp
chmod 1777 tools/rh9/rootfs/tmp tools/rh9/rootfs/var/tmp
test -x tools/rh9/rootfs/usr/bin/g++
test -x tools/rh9/rootfs/bin/sh
echo "Historical Red Hat Linux 9 toolchain rootfs is ready"

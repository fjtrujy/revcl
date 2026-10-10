#!/bin/sh
# Reproducible, noncanonical GCC 3.3.5 bootstrap for VUC i386 experiments.
# Requires Linux x86_64 with 32-bit runtime/development headers, curl,
# sha256sum and dpkg-deb. No installation into /usr is performed.
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: sh vuc/scripts/bootstrap_compat_gcc.sh /scratch/gcc335" >&2
    exit 2
fi

out=$1
mkdir -p "$out" "$out/root"
out=$(cd "$out" && pwd -P)
packages=$out
url=https://archive.debian.org/debian/pool/main/g/gcc-3.3

# SHA-256 verified against packages obtained from Debian's historical archive.
while read -r file checksum; do
    [ -n "$file" ] || continue
    if [ ! -f "$packages/$file" ]; then
        curl --fail --location --show-error --silent --retry 2 \
             "$url/$file" --output "$packages/$file"
    fi
    printf '%s  %s\n' "$checksum" "$packages/$file" | sha256sum --check --status || {
        echo "checksum failed: $file" >&2
        exit 1
    }
    dpkg-deb --extract "$packages/$file" "$out/root"
done <<'PACKAGES'
cpp-3.3_3.3.5-13_i386.deb 4e73b2009535996ab3db4f335bafc03f581a2ef493956da7a5455940ae301598
g++-3.3_3.3.5-13_i386.deb 1c23604fa7f73f3137048d7dce4843c692d264cab528addb524b1a1f844d0515
gcc-3.3-base_3.3.5-13_i386.deb f2121de4a654a240b3dc11669099f0499f0381751c8d9f28cdab676eae6c4c5a
gcc-3.3_3.3.5-13_i386.deb 88799d7e6221c323a5131639f9505780d779946ea44e3a1172833b71d47aea87
libstdc++5-3.3-dev_3.3.5-13_i386.deb dc07eca2459cf131099754ed639658787bc6281db3b004ccd3915014016beecd
libstdc++5_3.3.5-13_i386.deb 4e823f378f0d1d846b977f0f261bbacc11ed2c251ab05fe3c6dacba28a28b09f
PACKAGES

# Use the extracted compiler and C++ headers rather than host GCC/libstdc++.
# The Debian guest must supply i386 glibc development headers separately.
cat > "$out/g++-compat" <<EOF
#!/bin/sh
exec "$out/root/usr/bin/g++-3.3" \
    -nostdinc++ \
    -Wa,--32 \
    -isystem "$out/root/usr/include/c++/3.3" \
    -isystem "$out/root/usr/include/c++/3.3/i486-linux" \
    -isystem "$out/root/usr/include/c++/3.3/backward" \
    -isystem /usr/include/i386-linux-gnu \
    -isystem /usr/include \
    "\$@"
EOF
chmod +x "$out/g++-compat"
compiler_version=$("$out/g++-compat" --version)
printf '%s\n' "$compiler_version" | head -n 1

echo "Noncanonical compiler only. VUC requires GCC 3.3.3 20040125 prerelease."

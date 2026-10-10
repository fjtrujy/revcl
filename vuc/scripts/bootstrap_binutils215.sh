#!/bin/sh
# Recover a verified GNU binutils 2.15 i386 assembler for GCC 3.3.3 probes.
# This matches VUC's historical alignment encoding; it is NOT proof of the
# original VUC assembler/linker package or complete toolchain identity.
set -eu

if [ "$#" -lt 3 ] || [ "$#" -gt 3 ]; then
    echo "usage: sh vuc/scripts/bootstrap_binutils215.sh SCRATCH GCC333_ROOT CXX_HEADERS" >&2
    exit 2
fi

mkdir -p "$1"
scratch=$(cd "$1" && pwd -P)
compiler_root=$(cd "$2" && pwd -P)
headers=$(cd "$3" && pwd -P)

deb=binutils_2.15-6_i386.deb
sha=f4a73c5ad1cba4c61b38bd83e6b0e493cc6a1a41d30bc010ae4969617d12bc9b
if [ ! -f "$scratch/$deb" ]; then
    curl --fail --location --silent --show-error --retry 2 \
        "https://archive.debian.org/debian/pool/main/b/binutils/$deb" \
        --output "$scratch/$deb"
fi
printf '%s  %s\n' "$sha" "$scratch/$deb" | sha256sum -c
mkdir -p "$scratch/root" "$scratch/tool-bin"
dpkg-deb -x "$scratch/$deb" "$scratch/root"

cat > "$scratch/tool-bin/as" <<EOF
#!/bin/sh
export LD_LIBRARY_PATH="$scratch/root/usr/lib\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
exec "$scratch/root/usr/bin/as" "\$@"
EOF
chmod +x "$scratch/tool-bin/as"

cat > "$scratch/g++-jan25-as215" <<EOF
#!/bin/sh
exec "$compiler_root/build/gcc/xgcc" \\
    -B"$scratch/tool-bin/" -B"$compiler_root/build/gcc/" \\
    -x c++ -nostdinc++ \\
    -isystem "$headers" -isystem "$headers/i486-linux" \\
    -isystem "$headers/backward" \\
    -isystem /usr/include/i386-linux-gnu -isystem /usr/include \\
    -Wa,--32 "\$@"
EOF
chmod +x "$scratch/g++-jan25-as215"

LD_LIBRARY_PATH="$scratch/root/usr/lib" "$scratch/root/usr/bin/as" --version | head -n 1
"$scratch/g++-jan25-as215" --version | head -n 1
echo "Toolchain candidate installed; historical VUC binutils version still unverified."

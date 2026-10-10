#!/bin/sh
# Reconstruct a 25 January 2004 GCC 3.3 development compiler from public CVS history.
# This is a source-derived candidate, NOT an authenticated Debian 0pre3 package.
# Run inside a Linux x86_64 VM with 32-bit runtime support, make, bison, flex,
# curl, sha256sum, gcc, GNU binutils and glibc development files installed.
# Building the compiler only avoids the unsupported historical target runtime.
set -eu

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "usage: sh vuc/scripts/bootstrap_jan25_gcc.sh /scratch/gcc-jan25 [jobs]" >&2
    exit 2
fi

work=$1
jobs=${2:-2}
mkdir -p "$work"
work=$(cd "$work" && pwd -P)
revision=5b4de06ecdb5c4a1c232325aa293bd7b5f33d94f
source_name=gcc-$revision
source_sha=cc34f2966ceeb89f9e5687af6ce03c3ae686e53d03667a9fe77fd45e750be611
bison_sha=93a3264eed76aa85455cf6385fcad5f161112ad17535b0b52be889906d94df41

if [ ! -f "$work/gcc-jan25.tar.gz" ]; then
    curl --fail --location --retry 2 --show-error --silent \
        "https://codeload.github.com/gcc-mirror/gcc/tar.gz/$revision" \
        --output "$work/gcc-jan25.tar.gz"
fi
printf '%s  %s\n' "$source_sha" "$work/gcc-jan25.tar.gz" | sha256sum -c

if [ ! -f "$work/bison_1.875d-1_i386.deb" ]; then
    curl --fail --location --retry 2 --show-error --silent \
        https://archive.debian.org/debian/pool/main/b/bison/bison_1.875d-1_i386.deb \
        --output "$work/bison_1.875d-1_i386.deb"
fi
printf '%s  %s\n' "$bison_sha" "$work/bison_1.875d-1_i386.deb" | sha256sum -c

if [ ! -d "$work/$source_name" ]; then
    tar -xzf "$work/gcc-jan25.tar.gz" -C "$work"
fi
mkdir -p "$work/old-bison"
dpkg-deb -x "$work/bison_1.875d-1_i386.deb" "$work/old-bison"

# Old GCC has a macro that is no longer valid C under modern GCC (cast lvalue
# increment). This compatibility change is applied only to the scratch copy.
# Never patch the tracked upstream archive or VUC's reconstruction source.
obstack="$work/$source_name/include/obstack.h"
if grep -Fq '*((void **)__o->next_free)++' "$obstack"; then
    sed -i 's/\*((void \*\*)__o->next_free)++/\*((void \*\*)__o->next_free)/' "$obstack"
    sed -i 's/= ((void \*)datum);/= ((void *)datum); __o->next_free += sizeof(void *);/' "$obstack"
fi

# GCC 3.3 also relies on conditional lvalues, which modern GCC rejects in C.
# Select the original binding pointer first and dereference it as an lvalue.
decl="$work/$source_name/gcc/cp/decl.c"
if grep -Fq '? cp_function_chain->bindings' "$decl"; then
    sed -i '675,678c\#define current_binding_level (*(cfun && cp_function_chain->bindings ? \&cp_function_chain->bindings : \&scope_chain->bindings))' "$decl"
fi

mkdir -p "$work/build"
if [ ! -f "$work/build/Makefile" ]; then
    (cd "$work/build" && "../$source_name/configure" \
        --target=i686-linux-gnu --disable-bootstrap --disable-multilib \
        --disable-nls --enable-languages=c,c++ \
        --disable-libstdcxx --disable-libmudflap) \
        > "$work/configure.log" 2>&1
fi

(
    cd "$work/build"
    make -j"$jobs" all-libiberty
)

(
    cd "$work/build/gcc"
    BISON_PKGDATADIR="$work/old-bison/usr/share/bison" \
        make -j"$jobs" xgcc cc1plus BISON="$work/old-bison/usr/bin/bison" \
        BOOT_CFLAGS=-O0 CFLAGS=-O0
) > "$work/build.log" 2>&1 || {
    tail -n 30 "$work/build.log" >&2
    exit 1
}

headers=${VUC_CXX_HEADERS:-/tmp/vuc-gcc335/root/usr/include/c++/3.3}
cat > "$work/g++-jan25-candidate" <<EOF
#!/bin/sh
exec "$work/build/gcc/xgcc" -B"$work/build/gcc/" -x c++ \\
    -nostdinc++ -isystem "$headers" \\
    -isystem "$headers/i486-linux" -isystem "$headers/backward" \\
    -isystem /usr/include/i386-linux-gnu -isystem /usr/include \\
    -Wa,--32 "\$@"
EOF
chmod +x "$work/g++-jan25-candidate"
"$work/g++-jan25-candidate" --version | head -n 1
if [ ! -d "$headers" ]; then
    echo "WARNING: external compatibility C++ headers missing: $headers" >&2
fi
printf '%s\n' "Source-date candidate built; NOT an authenticated Debian GCC 3.3.3 0pre3 toolchain."

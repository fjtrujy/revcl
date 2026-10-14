#!/bin/sh
# Restore historical VUC compiler aliases after the Lima guest clears /tmp.
# Run inside revcl-oracle after building the pinned compiler candidates into
# the Linux guest's persistent home; never modifies the REVCL repository.
set -eu

cache=${1:-"$HOME/.cache/vuc-toolchain"}
cache=$(cd "$cache" && pwd -P)
scratch=/tmp

if [ ! -x "$cache/binutils215/g++-jan25-as215" ]; then
    printf '%s\n' "missing verified January GCC wrapper: $cache/binutils215/g++-jan25-as215" >&2
    exit 1
fi
if [ ! -x "$cache/gcc333-jan25/build/gcc/xgcc" ]; then
    printf '%s\n' "missing recovered GCC 3.3.3 frontend" >&2
    exit 1
fi
if [ ! -x "$cache/gcc333-jan25/build/gcc/cc1" ]; then
    printf '%s\n' "missing recovered historical C frontend (cc1)" >&2
    exit 1
fi
if [ ! -x "$cache/gcc333-jan25/build/gcc/cc1plus" ]; then
    printf '%s\n' "missing recovered historical C++ frontend (cc1plus)" >&2
    exit 1
fi

# Refuse to replace a real scratch directory; only restore absent symlinks
# or aliases already owned by this cache.
for pair in \
    "vuc-binutils215:binutils215" \
    "vuc-gcc333-jan25:gcc333-jan25" \
    "vuc-gcc335:gcc335"; do
    target=${pair%%:*}
    source=${pair#*:}
    alias="$scratch/$target"
    if [ -e "$alias" ] && [ ! -L "$alias" ]; then
        printf '%s\n' "refusing to overwrite scratch directory $alias" >&2
        exit 1
    fi
    if [ ! -d "$cache/$source" ]; then
        printf '%s\n' "missing cached directory $cache/$source" >&2
        exit 1
    fi
    ln -sfn "$cache/$source" "$alias"
done

if [ -d "$cache/gcc333-dec06" ]; then
    alias="$scratch/vuc-gcc333-dec06"
    if [ ! -e "$alias" ] || [ -L "$alias" ]; then
        ln -sfn "$cache/gcc333-dec06" "$alias"
    fi
fi

"$scratch/vuc-binutils215/g++-jan25-as215" --version | head -n 1
printf '%s\n' "Persistent VUC toolchain aliases restored; original-compiler identity remains unproven."

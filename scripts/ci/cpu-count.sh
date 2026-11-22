#!/bin/sh
# Print the number of online logical CPUs on Linux or macOS.
# Used for both CMake build parallelism and CTest parallel test execution.
set -eu
count=
# nproc honors Linux CPU affinity/cpuset restrictions when available.
if command -v nproc >/dev/null 2>&1; then
  count=$(nproc 2>/dev/null || true)
fi
case "$count" in ''|*[!0-9]*|0) count= ;; esac
if [ -z "$count" ] && command -v sysctl >/dev/null 2>&1; then
  count=$(sysctl -n hw.logicalcpu 2>/dev/null || true)
fi
case "$count" in ''|*[!0-9]*|0) count= ;; esac
if [ -z "$count" ] && command -v getconf >/dev/null 2>&1; then
  count=$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)
fi
case "$count" in
  ''|*[!0-9]*|0) count=1 ;;
esac
printf '%s\n' "$count"

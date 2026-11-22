#!/bin/sh
# Invoke the original GCC 3.2.2 from modern-host CMake using a pinned RH9 image.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
case "$(pwd -P)/" in
  "$ROOT/"*) ;;
  *) echo "Historical CMake build must be inside $ROOT" >&2; exit 1 ;;
esac
exec docker run --rm --platform linux/386 \
  --mount "type=bind,src=$ROOT,dst=$ROOT" \
  -w "$(pwd -P)" "${REVCL_RH9_IMAGE:-revcl-rh9}" /usr/bin/g++ "$@"

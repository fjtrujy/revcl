#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
exec docker run --rm --platform linux/386 \
  --mount "type=bind,src=$ROOT,dst=$ROOT" \
  -w "$(pwd -P)" "${REVCL_RH9_IMAGE:-revcl-rh9}" /usr/bin/strip "$@"

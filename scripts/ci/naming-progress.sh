#!/bin/sh
# Refresh or verify the checked-in naming inventories and summary.
set -eu

case "${1:---check}" in
  --update|--check) mode=${1:---check} ;;
  *) echo "usage: sh scripts/ci/naming-progress.sh [--update|--check]" >&2; exit 2 ;;
esac

root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd -P)
cd "$root"

scratch=$(mktemp -d "${TMPDIR:-/tmp}/revcl-naming-progress.XXXXXXXX")
trap 'rm -rf "$scratch"' EXIT HUP INT TERM

cc -std=c99 -D_DARWIN_C_SOURCE -Wall -Wextra -Werror -O2 \
  tools/naming-audit.c -o "$scratch/naming-audit"
cc -std=c99 -D_DARWIN_C_SOURCE -Wall -Wextra -Werror -O2 \
  tools/naming-progress.c -o "$scratch/naming-progress"

"$scratch/naming-audit" src > "$scratch/identifiers.tsv"
"$scratch/naming-progress" --report \
  "$scratch/identifiers.tsv" \
  docs/naming-fields.tsv docs/naming-declarations.tsv \
  docs/naming-manual-review.tsv docs/naming-progress-baseline.tsv \
  > "$scratch/progress.md"

if [ "$mode" = "--update" ]; then
  if ! cmp -s "$scratch/identifiers.tsv" docs/naming-identifiers.tsv; then
    cp "$scratch/identifiers.tsv" docs/naming-identifiers.tsv
  fi
  if ! cmp -s "$scratch/progress.md" docs/NAMING_PROGRESS.md; then
    cp "$scratch/progress.md" docs/NAMING_PROGRESS.md
  fi
  echo "Updated naming progress and lexical inventory."
else
  "$scratch/naming-progress" --require-complete \
    "$scratch/identifiers.tsv" \
    docs/naming-fields.tsv docs/naming-declarations.tsv \
    docs/naming-manual-review.tsv docs/naming-progress-baseline.tsv \
    > /dev/null
  if ! cmp -s "$scratch/identifiers.tsv" docs/naming-identifiers.tsv; then
    echo "Naming lexical inventory is stale: run sh scripts/ci/naming-progress.sh --update" >&2
    exit 1
  fi
  if ! cmp -s "$scratch/progress.md" docs/NAMING_PROGRESS.md; then
    echo "Naming progress report is stale: run sh scripts/ci/naming-progress.sh --update" >&2
    exit 1
  fi
  echo "Naming progress and lexical inventory are current."
fi

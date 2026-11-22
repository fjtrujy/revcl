# Naming progress (VCL branch)

Tracked naming-review snapshot. The **baseline** is the pre-refactor `main` source at commit `00bf1a1b` (22 November 2026); its lexical counts were re-scanned from the Git archive, while its declaration counts are from the original Clang review. The **current** numbers are generated from the source token scanner, the owner-qualified field/AST review queues and the curated provisional-type queue. Lower counts generally mean fewer unresolved naming candidates; they do not measure functional decompilation completeness.

| Area | Baseline | Current | Change |
| --- | ---: | ---: | ---: |
| Files with address-based filenames | 16 | 0 | -16 |
| Classes with numeric names and no source alias | 4 | 0 | -4 |
| Classes with provisional names (manual review) | 4 | 0 | -4 |
| Structs with provisional names (manual review) | 5 | 0 | -5 |
| Functions with address-based names | 436 | 0 | -436 |
| Fields with offset names (owner-qualified) | 614 | 0 | -614 |
| Parameters with short/generic names (AST review) | 879 | 0 | -879 |
| Local variables with short/generic names (AST review) | 2291 | 0 | -2291 |
| Address-suffixed globals/other identifiers | 85 | 0 | -85 |
| Explicit numeric placeholder identifiers | 4 | 0 | -4 |

**Direct function naming progress:** 436 of 436 original address-based function names have been eliminated from source; 0 remain. These are genuine C++ function renames, not preprocessor aliases.

**Numeric class naming:** no numeric class spellings remain in source; their replacement types are named directly.

**Field evidence retained:** 59 reviewed field resolutions: 48 unreferenced layout-only fields, 10 reset/copy-only fields and 1 observed union word. These records are in [the field resolution ledger](naming-field-resolutions.tsv). A zero offset-name count means the original naming queue is closed, **not** that the proprietary purpose of all reserved storage has been proven.

## How to update this in every naming commit

Run from the repository root:

```sh
sh scripts/ci/naming-progress.sh --update
sh scripts/ci/naming-progress.sh --check
```

Commit this report and any changed inventory TSVs **with the source renames**. Git tracks the table's evolution: `git log -p -- docs/NAMING_PROGRESS.md` shows the change in each commit. The baseline is fixed in `docs/naming-progress-baseline.tsv`; do not reset it to hide regressions. The check mode verifies that generated files have been refreshed.

## What is counted (and what is not)

- **Files:** source basenames matching historical address patterns.
- **Functions:** distinct address-coded identifiers still present in source. Semantic replacements are direct C++ renames, accepted only when the historical executable hash remains unchanged.
- **Classes:** distinct address-coded type names minus those with readable source-level typedefs.
- **Provisional classes/structs:** manually recorded `open` items in [the curated queue](naming-manual-review.tsv); mark `resolved` when evidence supports the name.
- **Fields:** distinct owning type and `unk_XX` field declarations in [the field queue](naming-fields.tsv).
- **Parameters/local variables:** short or generic declarations in [the AST review queue](naming-declarations.tsv).
- **Other globals/numeric placeholders:** distinct token spellings from [the lexical inventory](naming-identifiers.tsv); these can overlap the local/parameter queues, so do not sum the table into one grand total.

**Important limitations:** The lexical scanner does not prove that a name is meaningless. `i` in a short loop may be fine; a plausible-looking name can still be wrong. The two declaration queues were originally extracted using Clang AST and then manually pruned: **their counts change only when those TSVs are updated**. Review affected declarations after each rename, and periodically regenerate the AST queues. Zero manually flagged structs would not prove all structures are named. Completed original declaration reviews are retained in [the declaration resolution ledger](naming-declaration-resolutions.tsv). The VUC reconstruction is tracked independently on its own branch; its early candidate inventory is [here](naming-vuc-identifiers.tsv).

For evidence, confidence levels, ABI constraints and next targets, see [the naming audit](NAMING_AUDIT.md).

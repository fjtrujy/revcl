# VUC exact initialized-data milestone: six original buffers

Date: 2026-10-12. Branch: `vuc`.

## Independent original ELF data verification

A new strict acceptance manifest at
`vuc/config/exact_data_candidates.json` verifies six initialized
`.data` symbols against the **canonical ZIP-backed VUC 2004.1 ELF**.

| Original ELF symbol | Linked address | Original bytes | Verified |
| --- | --- | ---: | ---: |
| `line` | `0x08077040` | 1,024 | 1,024/1,024 |
| `word` | `0x08077440` | 1,024 | 1,024/1,024 |
| `inputFileName` | `0x08077860` | 256 | 256/256 |
| `outputFileName` | `0x08077960` | 256 | 256/256 |
| `inputPath` | `0x08077a60` | 256 | 256/256 |
| `asmBuffer` | `0x08077ba0` | 65,536 | 65,536/65,536 |
| **Total** | | **68,352** | **68,352/68,352** |

Each is an authentic original `STT_OBJECT` in a file-backed
`.data` section with the exact recorded address, size, and zero
initialization. None needs a relocation within its own bytes.

The reconstructed `vuc/src/GlobalBuffers.cpp` uses ordinary
C++03-compatible array declarations and zero initialization.
The recovered date-matched GCC 3.3.3 compiler is explicitly invoked
with `-O2 -fno-zero-initialized-in-bss`, which preserves the
initialized arrays in a file-backed `.data` section instead of
placing them in `.bss`. That option is a **compiler hypothesis**,
not a proven original VUC build flag. The source contains **no
original virtual addresses or copied original ELF byte arrays**.
The independent strict matcher checks original addresses exclusively
through the configuration manifests.

The aggregate **does not mean 68,352 additional instruction bytes**
and is not included in `vuc/config/exact_candidates.json`.

## Strict data gate

`vuc/scripts/match_data_candidates.py` is a standalone data-object
gate analogous to, but deliberately independent from, the existing
function-only candidate gate.

It validates each original identity, rejects duplicate object
identities and untrusted paths, compiles unique source/flag
combinations only once, and calls `match_data.py` to resolve and
check **all original named object bytes**, relocations where
applicable, section type, size, and symbol address. Any mismatched
byte causes the entire gate to **FAIL**. The emitted JSON report
uses `scope: explicit_original_named_data_objects_only`,
`whole_binary_score: null`, and does not imply executable identity.

Run:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_data_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/data-buffers-oct12
```

Expected: `6/6 exact named original data objects; 68352/68352
verified bytes`, exit code zero.

The host unit tests additionally verify the six original ELF
identities and **every original byte** from the authenticated ZIP,
the compilation cache, duplicate/path rejection, and mandatory
failure when even one byte of each object differs.

## Explicitly unproven properties

Byte equality of each **named object's initialized contents** does
not establish its absolute placement in a newly linked executable.

The original `line` and `word` happen to be contiguous, as do
the three file/path buffers. But `asmBuffer` is separated from
`inputPath` by other original global data, and compilation-unit
separation/link order may be different in the reconstructed source.
This gate does **not** reconstruct those intervening symbols or
the full `.data` section layout.

The original binary is **314,338 bytes** and has **165,440 bytes
of `.text`**. The accepted source currently covers **165/165
individually checked original function-symbol records,
11,283/11,283 candidate instruction bytes**, independently from
the six matching data objects. No correctly linked fully
reconstructed VUC ELF passes the final
`vuc/scripts/whole_elf_gate.py`. Complete byte-for-byte identity,
including all remaining functions, data, relocations, headers and
padding, remains the **mandatory final acceptance condition**.

## Reproduce host tests and executable separation

```sh
python3 -m unittest discover -s vuc/tests -q
# Current host suite: 76 passing tests

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/oct12-resume-165
```

The code and data gates **must remain separately reported**. The
final ELF gate must remain a failure until a newly compiled and
linked executable exactly matches the original ZIP member, not merely
one or both partial gates.

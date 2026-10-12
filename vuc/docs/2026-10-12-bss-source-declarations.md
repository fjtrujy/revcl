# Source-level evidence for original VUC uninitialized ELF objects

Date: 2026-10-12; branch `vuc`.

## Evidence checkpoint

The canonical original VUC 2004.1 ELF's `.bss` allocation is
`SHT_NOBITS`: it has **no file-backed bytes**. A normal data-byte
comparison cannot establish its source declarations or linker layout.
The new, separately named `match_bss_candidates.py` verifies original
symbols and independently compiled GCC 3.3.3 source definitions:

- The reference object must be a unique canonical original ELF
  `STT_OBJECT` in `.bss`, at its independently specified original
  virtual address, with the expected exact size.
- The candidate must be a defined `STT_OBJECT` in allocated, writable
  `SHT_NOBITS` (ordinary C++ global/static variable). The symbol
  must have the same name and size as the reference, with no duplicate.
- Sources must exist under the already accepted `vuc/src/` tree; no
  candidate source can escape the repository.
- The compiler flags are explicit, the output is an independent
  machine-readable JSON report, and the runner rejects malformed
  originals, changed sizes/sections, duplicate identity and missing
  candidate symbols.

The current manifest contains **12/12 matching uninitialized source
declarations** occupying **18,437 source allocation bytes**:

| Original source declaration | Original symbol(s) | Allocation bytes |
| --- | --- | ---: |
| IostreamGlobal_constantsTabStop.cpp | `constantsTabStop` | 4 |
| InstructionArena.cpp | `Instruction::instrData` | 4 |
| ScopeVectorLifecycle.cpp | `exportModule`, `enclosingFunID`, `enclosingLoopID`, `float4MemSymbols` | 21 |
| IRPredicatesEnvironments.cpp | `globalEnv`, `localEnv` | 4,104 |
| MLTempStringData.cpp | singleton `tsd` and its static initialization guard | 12,236 |
| MLGameStringTableLifecycle.cpp | singleton `gst` and its static initialization guard | 2,068 |
| **Total** | **12 distinct named original records** | **18,437** |

These source fragments already existed and are used for other
accepted original function/source verifications; this checkpoint
does not invent declarations or hard-code linked addresses in C++.

## Explicit limitations

The structural evidence **does not verify the addresses emitted by a
complete linker**, final `.bss` padding/ordering, original
translation-unit boundaries, or the zero-initialized runtime state
after the loader runs. It does not cover anonymous or unnamed BSS
allocations. The report therefore states:

```
"matched_executable_file_bytes": 0
"linked_original_addresses_reproduced": false
"whole_binary_match": false
```

Do not add these 18,437 source allocation bytes to the existing
relocation-aware initialized-data byte total. The original's
additional repeated local `std::ios_base::Init` objects are excluded:
without authenticated translation-unit provenance, matching a
same-named one-byte object is not enough to pin its unique original
linked virtual address. Imported dynamic-runtime copy-relocation
objects in `.bss` are excluded as well.

The separately gated baseline remains **166/166 functions and
11,314/11,314 checked text bytes**, and **244/244 initialized data
objects and 71,536/71,536 file-backed data bytes**. These selective
gates are necessary progress toward, but not proof of, byte-for-byte
equivalence of the full original executable.

## Reproduce

On the host:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/data_gap_inventory.py --summary-only
```

In the existing Linux VM:

```sh
r=/path/to/revcl/vuc
cc=/tmp/vuc-binutils215/g++-jan25-as215
python3 "$r/scripts/match_bss_candidates.py" \
  --compiler "$cc" \
  --out /tmp/vuc-binutils215/bss-oct12-confirm
```

The result must be **12/12 declarations** and 18,437 verified
allocation bytes, with zero matched executable file bytes.

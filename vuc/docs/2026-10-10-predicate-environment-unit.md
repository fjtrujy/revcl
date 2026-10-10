# Byte-exact VUC predicate/environment startup translation unit

Date: 2026-10-10. Branch: `vuc`.

## Result

The January 25, 2004 GCC 3.3.3 candidate and GNU as 2.15 now
reproduce the original six-function predicate/environment cluster
from a **single ordinary C++ translation unit**:

| Original symbol | ELF address | Exact instruction bytes |
| --- | --- | ---: |
| `IsFoldable(IR::Operator_Data*)` | `0x0804f5a0` | 41/41 |
| `IsCommutative(IR::Operator_Data*)` | `0x08050c50` | 66/66 |
| `__tcf_1` (globalEnv) | `0x080502a0` | 16/16 |
| `__tcf_2` (localEnv) | `0x080502b0` | 16/16 |
| `_GLOBAL__I__Z10IsFoldablePN2IR13Operator_DataE` | `0x08050b80` | **31/31** |
| `__static_initialization_and_destruction_0(int,int)` | `0x0804f4d0` | **193/193** |

The first four were already independently byte-exact; the final
two are the new matches, totaling **224** newly accepted instruction
bytes. The 16-byte `__tcf_0` iostream cleanup at `0x08050290` was
also already accepted from the generic iostream reconstruction and is
now emitted byte-exactly by this same candidate object.

The new source fragment is `vuc/src/IRPredicatesEnvironments.cpp`.
It consolidates the previous provisional `IRPredicates.cpp` and
`GlobalEnvironments.cpp` fragments, avoiding duplicate definitions.
The accompanying `MLHashTable` default constructor initializes every
one of the 513 buckets to null. The original ELF's startup code does
this for both tables, not just for one.

## Original linked object evidence

The canonical ZIP-backed ELF independently preserves these original
BSS objects and their addresses:

- `globalEnv`: `0x0808ac40`, 2052 bytes (513 x 4)
- `localEnv`: `0x0808b460`, 2052 bytes
- `std::ios_base::__ioinit`: `0x0808ac20`

The generated 193-byte helper registers the three original teardown
callbacks via `__cxa_atexit` and `__dso_handle`, initializing the
iostream object and both hash environments. All **13 supported ELF32
relocations** resolve to original symbols, callbacks and PLT targets.
The keyed 31-byte initialization wrapper calls this same linked helper,
adding one further checked relocation.

The two wrapper bytes that previously differed came from register
selection: compiling `IsFoldable()` alone generated `%eax` as the
temporary holding `0xffff`; the combined period-plausible translation
unit generates `%edx`, exactly as the original. The historical
compiler emits the complete 193-byte helper at the **original length
and with every byte equal**. This result depends on reconstructing
the actual global initializer relationships, not on inserting
assembly or artificial padding.

Compiler invocation for the accepted combined unit:

```sh
/tmp/vuc-binutils215/g++-jan25-as215 \
  -O2 -frename-registers -fuse-cxa-atexit \
  -ffunction-sections -falign-jumps=1 \
  -c vuc/src/IRPredicatesEnvironments.cpp \
  -o /tmp/vuc-binutils215/predicates-environments-accepted.o
```

The source, comments and compiler flags are **source-level
hypotheses**. We do not claim to have authenticated the original
Debian compiler patches, the exact historical header spelling or
the full linked executable. The word "translation unit" here means
the reconstructed compilation grouping is sufficient to reproduce
all seven described functions, not that every historical object
section is yet identical.

## Important matcher correction

This reconstruction uncovered a genuine ELF32 relocation matcher
error. The candidate object contains `localEnv` as a *defined named
STT_OBJECT* at candidate-local BSS offset `0x820`. Its original
target is explicitly `0x0808b460`. The old matcher added the
candidate `st_value` `0x820` again to that authenticated original
address, causing two false instruction-byte differences at offsets
53 and 54 of the 193-byte helper.

`linked_symbol_offset()` now treats both defined named
`STT_OBJECT` and `STT_FUNC` relocations as referring to the
configured original symbol base. `STT_SECTION` relocations still
retain section offsets and must resolve through the separately
verified original object/literal mapping. A regression unit test
covers a nonzero-offset `STT_OBJECT` and distinguishes that case
from a genuine section relocation.

**The correction does not waive or mask any original byte**: all 193
bytes, including the re-based 32-bit object address, still undergo
literal byte comparison against the original executable.

## Validated checkpoint and remaining work

The complete strict gate now accepts **135/135 original function
records, 6,985/6,985 checked code bytes, 303 resolved relocations,
36 original literal bytes**. The host test suite has 51 passing tests.
Original 298-entry text symbol status: **135 EXACT**, **10
EXPERIMENTAL**, **153 UNRESOLVED**.

Reproduction:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-135
```

**Complete ELF byte-for-byte equality is still mandatory and has
not yet been achieved.** Named-function equality does not establish
ELF data, relocation layout, padding, section ordering, linkage or
startup-library identity.

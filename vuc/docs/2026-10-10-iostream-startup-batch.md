# VUC per-translation-unit iostream startup and cleanup

## Exact checked scope

GCC **3.3.3 20040125 prerelease**, GNU **as 2.15** and
`-fuse-cxa-atexit` reproduce 20 separately addressed **16-byte
`__tcf_0` cleanup callbacks** against the ZIP-backed original VUC ELF.
Each callback resolves two relocations:

- the original `_ZSt8__ioinit` static object (20 *distinct* original
  addresses, even though the local ELF names are repeated);
- `std::ios_base::Init::~Init()` via its original PLT target
  `0x080499dc`.

The same ordinary source declaration `#include <iostream>` is used for
the *common compiler-generated callback*, not a claim that 20 complete
application translation units have been recovered.

The checked-in `vuc/scripts/iostream_cleanup_inventory.py` reads all
original named `__tcf_0` functions of size 16, checks their original
instruction structure, decodes the BSS object address and final
relative jump, validates each original `_ZSt8__ioinit` symbol in the
ELF symbol table, then writes the 20 distinct
`vuc/config/iostream_cleanup_<address>_probe.json` manifests.
No object addresses or instructions are fabricated by the generator.

The 21st original `__tcf_0` record is **22 bytes** long at
`0x08071e00` and calls a hash-table cleanup instead. It is not
represented as an iostream callback and remains unresolved.

## Why local symbol identity now includes original address

The original VUC ELF repeats the local name `__tcf_0` for distinct
translation units. The former candidate-runner duplicate guard keyed
only by name and would reject authentic per-TU records. The new guard
keys `(mangled_symbol_name, original_elf_virtual_address)`.

Two manifests naming the **same address** still fail as duplicates.
Two original symbols with the same local name and different addresses
are separately verified, with their own original BSS symbols and
relocation targets. Unit tests verify both cases, as well as rejected
unknown/ambiguous object symbols and corrupted original instruction
patterns.

The per-TU iostream object addresses range from `0x0808aac0` to
`0x0808bcbc`. Each is checked by its actual original ELF `STT_OBJECT`
address. The candidate's statically emitted object offset is rebased
through the source symbol, not through a guessed whole-`.bss` base.

## Global constructor wrapper

The original `_GLOBAL__I_constantsTabStop` at `0x0804a700`
matches **31/31 bytes**, including its call to the original
`__static_initialization_and_destruction_0(int,int)` helper at
`0x08049af0`. The original `constantsTabStop` is a four-byte BSS
symbol, represented by a provisional `int constantsTabStop` global.

Compiling with `-ffunction-sections` makes the outgoing call to the
helper an explicit ELF32 `R_386_PC32` relocation; the strict matcher
resolves that relocation against the **original helper address**.
Without it, a same-section local call is already finalized at an
unlinked object-local displacement and cannot legitimately be compared
with its different original linked displacement.

This flag is a *matching-toolchain hypothesis*, **not evidence** that
the original compiler used `-ffunction-sections`, nor proof that
we have reconstructed the complete original translation unit.

## All fourteen 66-byte iostream initializer helpers

The canonical ELF contains **14** copies of the same-shaped
`__static_initialization_and_destruction_0(int,int)` function with
**66 instruction bytes** each. This historical GCC-generated code:

1. Checks the compiler's magic initialization parameters.
2. Calls `std::ios_base::Init::Init()` on that translation unit's
   independently addressed `_ZSt8__ioinit` object.
3. Registers the translation unit's own `__tcf_0` callback with
   `__cxa_atexit` and the original `__dso_handle`.

Compiling ordinary `#include <iostream>` with source-date GCC
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections
-falign-jumps=1` produces **66 matching instruction bytes** in
every case after mapping these five original relocations:
`std::ios_base::Init::Init()`, `_ZSt8__ioinit`,
the corresponding original `__tcf_0`, `__dso_handle`, and
`__cxa_atexit`.

For `constantsTabStop`, the helper at `0x08049af0` comes from
the accepted named-global source fragment, alongside its exact
31-byte `_GLOBAL__I_constantsTabStop` and 16-byte cleanup
callback. The other 13 66-byte helpers are accepted from the
isolated iostream-source fragment.

The generator `iostream_cleanup_inventory.py --write` now also
derives and verifies these **13 additional** helper configurations
directly from the original ELF. It rejects unexpected instruction
patterns, original PLT targets, absent cleanup-callback records,
and unmapped original BSS symbols. It deliberately excludes
`0x08049af0`, which has its own named-global manifest.

Original initializer helpers larger than 66 bytes remain unresolved.
The 66-byte helper family and its associated 16-byte teardown
callbacks are compiler-generated lifecycle scaffolding, **not
evidence of recovering the original application's class logic**.

Other original wrappers for `matchStack`, `exportModule` and
`script_debug` each have a 31-byte body, but the tested source
fragments match **29/31 bytes**, with a different register selected for
the `0xffff` immediate. They remain in `vuc/probes/`, marked
EXPERIMENTAL, and are **not** included in accepted source.

## IsFoldable control-flow milestone

Separately, `IsFoldable(IR::Operator_Data*)` at `0x0804f5a0`
now matches **41/41 bytes**. The critical insight was writing the
original predicate as successive `if / else if` branches that set a
boolean result for expression kind values 13, 12, 11 and 10.
A compact logical OR is semantically equivalent but GCC collapses
its comparisons to a short range test, producing a **non-exact**
function. No machine instructions or manual padding were inserted.

## Aggregate strict gate

Together with previous accepted milestones:

| Category | Count |
| --- | ---: |
| Original function-symbol records accepted | **80 / 80** |
| Exact instruction bytes in accepted records | **3,318 / 3,318** |
| Resolved ELF32 relocations | **170** |
| Per-manifest verified literal bytes | **29** |
| Non-exact experimental function symbols | 8 |
| Other unresolved function symbols | 210 |
| Total original text function symbols | 298 |

The counts include **20 structurally identical compiler-generated
iostream teardown callbacks and 14 similar initialization helpers**.
They are not 34 distinct reconstructed application algorithms.
None of these numbers is a claimed whole-ELF
match percentage.

## Reproduction

```sh
python3 vuc/scripts/iostream_cleanup_inventory.py --write
python3 vuc/scripts/coverage.py --write
python3 -m unittest discover -s vuc/tests -q

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-80
```

All 80 accepted records must compare exactly. The exploratory
manifest must fail, because partially reconstructed functions
remain. The mandatory final task is still **byte-for-byte matching
of the entire original executable**, including linker layout,
ELF metadata, original data, runtime and all remaining functions.

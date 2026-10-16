# VUC: VFormatString recovered 238/238 from historical assert and signed Max()

Date: 2026-10-16. Original is the immutable VUC 2004.1 Linux i386 ELF
inside the tracked ZIP. Original function
`MLTempStringData::VFormatString(const char *,char *)`:
`_ZN16MLTempStringData13VFormatStringEPKcPc`, at `0x08071a50`,
**238 original instruction bytes**.

## From unresolved to byte-exact

The earlier semantic candidate
`vuc/probes/ml_temp_vformat.cpp` and 260 historical GCC recipes
recovered original-size output but failed the strict checker because
the emitted literal section contained 32 unowned alignment bytes.
The previous code also manually called `__assert_fail`, thereby
changing compiler string-emission order.

A new bounded reconstruction in
`vuc/probes/evaluate_vformat_assert_macro_owner_oct16.py`
models actual C++03 `assert` macros, a genuine inline
`MLTempStringData::Reserve(int)` helper and evidence-backed
original file/line diagnostics (`../megalib/MLGameString.cpp`
at lines 145 and 200). The `#line` directives reproduce
**original ELF diagnostic evidence**, not matching-only NOPs,
instruction bytes or arbitrary padding.

Historical GCC 3.3.3 now emits the original source-unit literal
cluster in the exact original order:

| Relative compiler `.rodata` offset | Original ELF address | Literal |
| ---: | --- | --- |
| `+0x00` | `0x08076860` | `char* MLTempStringData::VFormatString(const char*, char*)` |
| `+0x40` | `0x080768a0` | `char* MLTempStringData::Reserve(int)` |
| `+0x65` | `0x080768c5` | `numWritten >= 0` |
| `+0x75` | `0x080768d5` | `../megalib/MLGameString.cpp` |
| `+0x91` | `0x080768f1` | `Max() > strLen` |

The entire **160-byte literal section**, including the authentic
seven zeros after the first signature, is byte-identical to the
original at `0x08076860..0x080768ff`. Unlike the earlier
rejected candidates, no bytes remain unmapped.

The first historically compiled, original-sized object then scored
**236/238 code bytes** with ten authentic relocations and all
160 literal bytes verified. The only mismatches were at original
relative offsets **+49** and **+62**:

| Offset | Original opcode | Initial candidate opcode |
| ---: | --- | --- |
| `+49` | `jge` (`0x7d`) | `jae` (`0x73`) |
| `+62` | `jle` (`0x8e`) | `jbe` (`0x86`) |

The original performs signed comparisons in the Reserve bounds
check. The provisional source declared `Max()` as returning
`unsigned`, making the compiler choose unsigned jumps.
Changing its source declaration to
`int Max() const { return 0x1fbf; }` reproduced both original
opcodes without touching generated machine code. This establishes
**signed return semantics for the inlined Max helper in this body**,
not complete independent proof of the original header layout.

## Historical acceptance

The accepted source is `vuc/src/MLTempVFormatExact.cpp`;
the accepted manifest is
`vuc/config/ml_temp_vformat_assert_exact_oct16.json`.

Under `-O2 -frename-registers -falign-jumps=1`, independently
compiled with both recovered January 25 and December 6, 2003/2004
GCC 3.3.3 prerelease snapshots and GNU as 2.15:

| Gate | Jan25 | Dec06 |
| --- | ---: | ---: |
| Function length | 238/238 | 238/238 |
| Original-offset instruction bytes | **238/238** | **238/238** |
| Original relocation sites | **10/10** | **10/10** |
| Authenticated full `.rodata` section | **160/160** | **160/160** |
| Byte exact | **YES** | **YES** |

The ten relocation sites target the original `strlen`,
`vsprintf`, `__assert_fail` and the original authenticated
literal section. No custom assembly, object patching, fake addresses,
or artificial matching bytes were used. The source remains a
function-level historical reconstruction; whole-translation-unit
linker layout and full ELF equality are not established.

## Reproduction

Inside the existing `revcl-oracle` Linux VM:

```sh
R=/path/to/revcl/vuc
for snapshot in jan25 dec06; do
  /tmp/vuc-binutils215/g++-${snapshot}-as215 \
    -O2 -frename-registers -falign-jumps=1 \
    -c "$R/src/MLTempVFormatExact.cpp" \
    -o "/tmp/vuc-vformat-accepted-${snapshot}-oct16.o"
  python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-vformat-accepted-${snapshot}-oct16.o" \
    --config "$R/config/ml_temp_vformat_assert_exact_oct16.json"
done
```

The new host oracle
`vuc/tests/test_ml_temp_vformat_exact_contract.py`
guards original ELF signed jumps, literal ownership and manifest
provenance. The exact list increases to **205 accepted functions**,
and accepted verified instruction bytes increase from **15,855**
to **16,093**. Original named-function inventory is now
**205 EXACT / 39 EXPERIMENTAL / 54 UNRESOLVED** of 298.

Separate initialized-data and BSS gates remain required, and
whole-linked-ELF byte identity remains unestablished.

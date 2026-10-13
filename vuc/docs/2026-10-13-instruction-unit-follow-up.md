# Instruction arena destructor: genuine full-source-unit control

Date: 2026-10-13; branch `vuc`.

The pending `__tcf_1` Instruction arena cleanup at original `0x08051980`
remains **45/46**, differing only at offset 42: original `pop %ecx`,
GCC 3.3.3 candidate `pop %eax`. Both candidate and original function
have the same length, the same algorithm, and three correctly resolved
relocations.

A new control, `vuc/probes/instruction_full_source_unit_oct13.cpp`,
combines these genuine ordinary C++ definitions in one compilation unit:

- `Instruction::ToString()` (original `0x08051900`)
- `Instruction::operator new(std::size_t)` (original `0x08051910`)
- `Instruction` arena static constructor and destructor, including the
  genuine compiler-generated startup code

Using the existing January 2004 GCC 3.3.3 + GNU as 2.15 wrapper with
`-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections` produces:

| Function | Original address | Result |
| --- | --- | --- |
| `Instruction::ToString` | `0x08051900` | 9/9 exact |
| `Instruction::operator new` | `0x08051910` | 86/86 exact |
| static initialization helper | `0x080514a0` | 138/138 exact |
| global initializer wrapper | `0x080519b0` | 31/31 exact |
| cleanup `__tcf_1` | `0x08051980` | 45/46 partial |

This experiment verifies that the recovered C++ declarations can coexist
in one original-like source unit without disturbing four accepted bodies.
It also rules out simply adding the adjacent genuine `ToString` and
allocator definitions as the missing cause of the destructor's final
register choice. It does **not** prove the full original source unit or
create four new function acceptances (those are already in the manifest).
No speculative compiler override or source-unrelated padding was retained.

Reproduce without touching accepted production source:

```sh
limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  /tmp/vuc-binutils215/g++-jan25-as215 \
    -O2 -frename-registers -fuse-cxa-atexit -ffunction-sections \
    -c "$R/probes/instruction_full_source_unit_oct13.cpp" \
    -o /tmp/vuc-binutils215/instruction-full-unit-oct13.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/instruction-full-unit-oct13.o \
    --config "$R/config/instruction_arena_cleanup_probe.json"
'
```

Next experiments should inspect source-level destructors and
GCC 3.3.3 `.27.peephole2` / `.28.rnreg` dump behavior, not fabricate
an instruction encoding or accept the 45/46 partial as exact.

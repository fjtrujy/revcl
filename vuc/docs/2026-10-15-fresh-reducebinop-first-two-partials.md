# Fresh VUC ReduceBinOp reconstruction: two 300/315 original-length templates

Date: 2026-10-15. Original executable: authenticated VUC 2004.1 Linux
ELF32 i386. Target symbols had **no previous selected candidate**
under `vuc/src` or `vuc/probes` at the start of this round.

## Original control flow reconstructed

The original two adjacent weak C++03 template instantiations are:

- `ReduceBinOp<unsigned int>` at `0x08050540`, **315 bytes**.
- `ReduceBinOp<int>` at `0x08050680`, **315 bytes**.

Both have a common operation layout within `IR::Operator_Data` at
offset +4. ELF disassembly exposes a descending branch chain for
operator numbers 13, 12, 11, 10, 7, 6, 5, 4, 3, 2, 1, 0,
then special fallthrough cases 9, 8. These correspond to:

| Opcode | Operation |
| --- | --- |
| 0 | logical OR |
| 1 | logical AND |
| 2 | less than or equal |
| 3 | greater than or equal |
| 4 | less than |
| 5 | greater than |
| 6 | unequal |
| 7 | equal |
| 8 | bitwise OR |
| 9 | bitwise AND |
| 10 | division |
| 11 | multiplication |
| 12 | subtraction |
| 13 | addition |

The difference between signed and unsigned instantiations is *real*:
signed comparisons use signed setcc operations and `idiv`, while
unsigned uses unsigned setcc and `div`. The source uses the
real template parameter `T` instead of simulating those instructions.

A notable original-era quirk: **unrecognized opcode branches return
true without assigning `out`**. The first source draft returned false
for an unrecognized opcode. The original disassembly's shared return
path and GCC's emitted code instead support the successful-return
behavior, which is preserved even though it might seem like a bug.

## Historical GCC strict evidence

Source: `vuc/probes/ReduceBinOpNewOct15o.cpp`.

Selected flags: `-O2 -frename-registers -falign-jumps=8`, preserving
the actual GCC 3.3.3 generated branch/alignment pattern. Using both
independently recovered compiler snapshots (2003 Dec06 and 2004 Jan25)
with GNU as 2.15 produces **exact original size** and **300/315**
original-offset instruction bytes for each specialization. These
functions have **zero relocations**, so the relocations-aware strict
gate validates zero original relocation sites.

The only mismatching offsets are **+60** and **+258..+271**, exactly
**15 bytes** for *each* function. Original assembly places a
six-byte alignment before multiplication; GCC's reconstructed
source places it before division. Its branch displacement at +60
naturally follows the different landing placement. The arithmetic,
logical operators, comparison semantics and surrounding function
instructions otherwise match. The padding is compiler-emitted; no
assembly, fabricated padding, forced register names, raw opcodes,
or executable byte patches were used.

A first focused source exploration tested 16 switch/if/
return-value/operand-order patterns. A real if/else chain uniquely
reconstructs the original branch sequence; ordinary switch statements
produce radically shorter jump-table code. A narrow compiler
jump/label/loop alignment test (25 combinations) confirmed that
changing generic block-alignment settings does **not** move the
six bytes to their original location without also changing original
length or more code. Avoid broad compiler flag grids until authentic
source-unit predecessors or a new source semantic hypothesis are known.

The independently tracked partial manifests are
`vuc/config/reduce_binop_unsigned_partial_oct15o.json` and
`vuc/config/reduce_binop_signed_partial_oct15o.json`. Both remain
outside `exact_candidates.json`; **300/315 is NOT byte-exact**.
They increase the 298-function inventory from **204 EXACT /
27 EXPERIMENTAL / 67 UNRESOLVED** to **204 EXACT /
29 EXPERIMENTAL / 65 UNRESOLVED**.

Reproduction:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_reduce_binop_new_sources_oct15o.py"
# Selected source with historical compiler + strict matcher:
limactl shell revcl-oracle -- /bin/bash -lc '
  R=/path/to/revcl/vuc
  /tmp/vuc-binutils215/g++-jan25-as215 \
    -O2 -frename-registers -falign-jumps=8 \
    -c "$R/probes/ReduceBinOpNewOct15o.cpp" \
    -o /tmp/vuc-binutils215/ReduceBinOpNewOct15o.o
  for suffix in unsigned signed; do
    python3 "$R/scripts/match_probe.py" \
      --object /tmp/vuc-binutils215/ReduceBinOpNewOct15o.o \
      --config "$R/config/reduce_binop_${suffix}_partial_oct15o.json"
  done
'
```

The original source-level source and remaining padding provenance
are still unresolved; the exact linked VUC executable remains unbuilt.

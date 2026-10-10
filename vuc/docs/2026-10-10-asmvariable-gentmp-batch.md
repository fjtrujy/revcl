# Byte-exact VUC AsmVariable STL copies and instruction temporary names

Date: 2026-10-10. Branch: `vuc`.

## Exact verified milestone

Compared with the previous 136-function checkpoint, five more
application/STL/lexer functions now reproduce **every byte** of their original
canonical VUC ELF instruction bodies. The expanded strict gate passes
**141/141 accepted original function-symbol records**, **7,471/7,471
instruction bytes**, **321 resolved original ELF32 relocations** and
**78 verified original literal bytes**.

| Original function | Address | Exact bytes |
| --- | --- | ---: |
| `InstructionList::GenTmp(char const*)` | `0x08051840` | 85/85 |
| `std::__uninitialized_copy_aux<AsmVariable const*,AsmVariable*>()` | `0x0806e280` | 67/67 |
| `std::__uninitialized_copy_aux<AsmVariable*,AsmVariable*>()` | `0x08051450` | 74/74 |
| `yyFlexLexer::yy_delete_buffer(yy_buffer_state*)` | `0x08061840` | 87/87 |
| `InstructionList::GenLabel(char const*)` | `0x080517e0` | 85/85 |

All three are **ordinary historical C++ reconstructions**, using the
January 25, 2004 prerelease GCC 3.3.3 backend and GNU as 2.15.
No handwritten assembly, register forcing, generated instruction
arrays, injected padding or skipped mismatching bytes are involved.

## GenTmp and the original literal address

`InstructionList::GenTmp()` constructs an `MLTempString`
with `"%s_%03d"`, the supplied name, and a post-incremented static
integer initialized to one; it converts the temporary into an
`MLGameString`. Its original ELF code writes through the hidden
C++ return-object pointer, and the original counter is named
`_ZZN15InstructionList6GenTmpEPKcE2id` at `0x08077844`.

The function already produced **85/85 bytes** when built with the
still-non-exact `GenLabel()` hypothesis, but that candidate used a
combined `.rodata` section containing both name-format strings.
The prior experimental manifest mapped its section start to
`0x0807266a`, the *GenLabel* literal. When building `GenTmp`
in the accepted `InstructionList.cpp` unit instead, its entire
eight-byte `.rodata` contains only `"%s_%03d\0"`, which the
authenticated original ELF stores at **`0x08072677`**.

Correcting the independently verifiable literal address removes
the false literal-layout failure. The same accepted source now emits:

- `GenTmp()`: **85/85**, five relocations, eight literal bytes;
- `Print()`: **87/87** with its established flags;
- `Add()`: **55/55** with `-falign-jumps=1`;
- `AddL()`: **107/107** with `-falign-jumps=1`.

The additional `GenLabel()` branch now passes its full exact check,
including the independently named counter and its original format literal.
The two label generators share the same C++ class fragment without
artificial assembly or padding. Both are accepted only after compiling
from `vuc/src/InstructionList.cpp` and resolving their original relocations.

The lexer buffer-delete function also passes **87/87**, after the
ordinary source integration into `vuc/src/FlexLexer.cpp`. The original
`free()` calls and conditional buffer-ownership path are verified; the
complete historical lexer class and its translation-unit layout remain
unresolved.

## AsmVariable's default copy semantics

The original VUC STL helper implementations are instantiations of
GNU GCC 3.3's `std::__uninitialized_copy_aux` template for an
eight-byte `AsmVariable` element and historical
`__gnu_cxx::__normal_iterator` types. Their original bodies
copy two 32-bit words per entry, and the original compiler
**loads both words before writing either**.

A manually written `AsmVariable(const AsmVariable&)` copying fields
one after the other instead emitted interleaved load/store
instructions. Multiple prior probes with explicit assignment
orders produced correct-sized but non-exact functions (at best
46/67 and 55/74 matching bytes).

The exact source `vuc/src/AsmVariableUninitializedCopy.cpp`
declares a conventional two-word class with a user-provided
default constructor but **no explicit copy constructor**:

```cpp
struct AsmVariable {
    unsigned a;
    unsigned b;
    AsmVariable() : a(0), b(0) {}
};
```

GCC therefore synthesizes a memberwise copy constructor which
loads both words before assigning them and matches both original
template instantiations, **67/67 and 74/74 bytes**. This recovers
the relevant copy ABI and element width, but the original member
names and constructor spelling remain provisional.

The source sweep `vuc/probes/evaluate_asmvariable_copy.py`
independently evaluates several period-plausible aggregate and
implicit/explicit copy representations, using the same strict
original address and instruction bytes. Several related aggregate
declarations also reproduce both exact functions; only the
conventional implicit copy form is accepted.

## Regression and limits

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-141
```

The host suite currently has **56 passing unit tests**.
The canonical 298-original-function ledger is now **141 EXACT,
11 EXPERIMENTAL, 146 UNRESOLVED**. Function-level results do **not**
represent whole-linked-executable completeness. An additional strict
`vuc/scripts/whole_elf_gate.py` requires complete byte equality
for a rebuilt ELF when one exists. **No reconstructed full executable
has yet passed that gate.** Original Debian compiler patches,
linker inputs, remaining source, initialized data and ELF metadata
are still incomplete.

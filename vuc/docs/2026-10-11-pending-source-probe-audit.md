# VUC pending-hypothesis audit and compiler-register investigations

Date: 2026-10-11. Branch: `vuc`.

## Accepted baseline and non-acceptance policy

The canonical VUC 2004.1 ELF continues to have **155/155 individually
accepted exact original functions**, **9,372/9,372 compared instruction
bytes**, **402 independently resolved ELF32 relocations** and **132
verified literal bytes**. The original ELF text-symbol inventory is
**155 EXACT, 9 EXPERIMENTAL and 134 UNRESOLVED**, of 298 named functions.
The strict full candidate gate is necessary but does not imply
a full linked-executable match. All 60 host tests pass.

The current investigation did **not** promote any partial
function or source-unit hypothesis.

## Reproducible inventory

`vuc/probes/audit_pending_candidates.py` uses each pending manifest's
historical compiler flags, compiles candidate C++ objects in the
existing Linux VM, and uses the same strict relocation-aware function
matcher as the accepted gate. It deliberately does not modify the
original executable, source tree, or acceptance manifest.

After the exact C1 formatted-string constructor was accepted, a
new pending-probe audit identified only **two additional exact
candidate configurations**:

- `vuc/config/list_dispatch_bind_var_probe.json` verifies an
  original function **already accepted** at `0x0804edd0`.
- `vuc/config/ml_string_table_cleanup_probe.json` verifies the
  original 22-byte local cleanup thunk at `0x08071e00`,
  **already accepted** by `ml_game_string_table_cleanup_probe.json`.

They are duplicate hypotheses of accepted *original symbol
addresses*, **not two new independently matched functions**.
A raw probe-file count must not be confused with exact original
function coverage.

## Value arithmetic register allocation

`Value::operator*(Value const&) const` compiles to the exact original
**222-byte length**, but only **219 instruction bytes match**.
The three differences are at original offsets **155, 162, 164**:
the original integer-multiplication block loads the temporary
integer into `EDX`, multiplies via `EDX`, and stores via `EDX`.
The current GCC 3.3.3 source chooses `ECX` at those points.
The surrounding switch, 4-component floating-point multiply,
hidden return-value ABI and original `TypeConvert` relocation
match. Natural C++ source variants (signed/unsigned fields,
temporary variables, operand order, explicit multiplication)
and a large historical GCC flag sweep did not resolve these three
register-encoding bytes. There is no fabricated assembly or
register-forcing workaround in accepted source.

For `Value::operator/(Value const&) const`, original disassembly
establishes **signed integer** `idiv` on type 1, ordinary scalar
float division on type 3, and repeated `1.0f / denominator`
followed by multiplication for the four-element vector case.
`value_division_reciprocal.cpp` reproduces those semantics and
much of the x87 instruction structure, but the original 235-byte
control-flow and label alignment are not recovered. An alignment
sweep can generate **235-byte** variants, but they match only
**174/235 bytes**; they are unaccepted.

The neighboring `Value::operator+()` hypotheses can emit the
original 219-byte length using several jump alignment values,
but best tested 219-byte forms match only 172 bytes. Three bytes
of label/padding differences in the original case-3 block remain
unexplained; no manual padding is permitted.

## AST printer original translation-unit hypothesis

The original `Print(ASTFunCall*, std::ostream&)` body is 98 bytes.
The provisional ordinary source matches **96/98 bytes** under
`-O2`, with the same seven ELF relocations and all three
original string literals verified. The differences are initial
register encodings for the callee load. Compiling this
ordinary source together with the **already accepted**
`Print(ASTNodeList<T>&)` template specializations
(`print_fun_with_lists_before.cpp` and
`print_fun_with_lists_after.cpp`) did not correct them.
Reconstructed source-unit grouping alone is not sufficient
to resolve all historical compiler register decisions.

## Source provenance and DWARF limitation

`vuc/config/dwarf_units.json`, extracted directly from the
canonical original ELF by `dwarf_inventory.py`, contains
**four DWARF compilation-unit roots**. They are the glibc
startup sources `start.S`, `init.c`, `crti.S`, and `crtn.S`.
There are **no application compilation-unit DIEs** in this
DWARF inventory. Accordingly, it would be inaccurate to
claim that original application class layout, inline source,
local variable names, or method bodies can be obtained from
DWARF alone. The retained original `.symtab` and source-file
symbols are valuable, but they do not replace application DWARF.

`vuc/config/source_units.tsv` separately reconstructs original
source-file provenance from local ELF file symbols. It is evidence
for source grouping, not recovery of lost debug type declarations.

## Reproduction and next evidence targets

```sh
python3 -m unittest discover -s vuc/tests -q

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/resume-155

limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/audit_pending_candidates.py
```

Prioritize genuine C++ reconstruction of original control-flow
and real historical toolchain differences, especially the
unrecovered Debian GCC 3.3.3 `0pre3` patchset and original
translation-unit order. The remaining function records,
data/relocations, and full binary linkage are necessary before
the **mandatory byte-for-byte match of the complete original
executable** can be asserted.

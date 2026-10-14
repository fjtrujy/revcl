# VUC unresolved AST/IR batch: AssembleBinOp dispatch partial

Date: 2026-10-14. Branch: `vuc-unresolved-oct14`.
Parent baseline: `c009140` (195 accepted exact functions). Do not mix
this partial with the earlier Value, scanner, printer or Instruction probes.

## New genuine-source partial: 162/193 `AssembleBinOp`

Target: `_Z13AssembleBinOpRKN2IR8RegisterEPNS_9Expr_DataER15InstructionList`,
original VUC address **0x0805a3f0**, length **193**.

The original ELF's argument stack layout proves the source-level call
contract: hidden return pointer (+8), `const IR::Register &` (+12),
`IR::Expr_Data *` (+16), and `InstructionList &` (+20). The value-return
`IR::Register` is an eight-byte object: an `MLGameString` at offset 0 and
channel bits at offset 4. GCC emits a by-value sret call to a specialist,
followed by `sub $4, %esp` for the callee-popped hidden return argument.

The dispatch is structurally clear in the original instructions:

| Type kind | Target |
| --- | --- |
| 1, 2 | `AssembleBinOpInt(...)` at `0x08058640` |
| 3, 4 | `AssembleBinOpFloat4(...)` at `0x08056210` |
| Other | Out-of-line `MLGameString` copy + channel field copy |

The initial `switch` source in
`vuc/probes/assemble_binop_dispatch_oct14.cpp`, compiled with original
January 2004 GCC 3.3.3 and binutils 2.15, emits **153** bytes:
identical-case folding merges the dispatch arms and is not an authentic
193-byte binary reconstruction.

The validated partial is
`vuc/probes/assemble_binop_dispatch_partial_oct14.cpp` with the dedicated
experimental manifest
`vuc/config/assemble_binop_dispatch_oct14_probe.json`. Its historical
`-O2 -frename-registers -falign-jumps=4` object matches **162/193 original
instruction bytes** with **four authenticated relocations**, and the exact
original emitted function size. Strict matching still reports **31
mismatching offsets**:

- `+0x30` is the signed 8-bit destination of the jump to type 4.
- `+0x91..+0xae` is the float-case layout region. The original starts the
  kind-4 block at `+0x91` and pads six bytes before the kind-3 block
  at `+0xb0`. The reconstruction pads three bytes *before* the kind-4
  block (starting `+0x94`), then three before kind-3. All call arguments
  and target relocations remain semantically valid.
- `+0xc0` is the last branch displacement of the common float return.
  Both functions occupy 193 bytes in total.

The original branch sequence tests kinds **3, 4, 2, 1** individually,
then copies the target on the default path. The natural four-if C++ source
in this lexical order matches **131/193** bytes, before investigating
historical basic-block frequency.

One **diagnostic, unproven** GCC-specific source hypothesis,
`__builtin_expect(kind == 1, 0)`, changes original-sized output to
**162/193**. This extension existed in the historical compiler, but
there is **no evidence that the 2004 original developer wrote it**;
the likelihood annotation is only a way to inspect genuine GCC block
alignment behavior. Even if a variant achieved all instruction bytes,
it would need source-level authenticity before acceptance under the VUC
exact-only policy. The experiment neither injects NOPs, asm, dummy
predecessors, relocation addresses, nor binary byte patches.

### Reproduction

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  C=/tmp/vuc-binutils215/g++-jan25-as215
  "$C" -O2 -frename-registers -falign-jumps=4 \
    -c "$R/probes/assemble_binop_dispatch_partial_oct14.cpp" \
    -o /tmp/vuc-binutils215/assemble-binop-saved-partial-oct14.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/assemble-binop-saved-partial-oct14.o \
    --config "$R/config/assemble_binop_dispatch_oct14_probe.json"
'
```

The matcher independently reported:
`{"length":193,"exact_bytes":162,"exact":false,"relocations":4}`.
The complete mismatch offsets are `48, 145-150, 152-174, 192`.

### Source-level controls already eliminated

- `evaluate_assemble_binop_dispatch_oct14.py`: 24 switch orders, 24
  conditional orders, grouped switch arms, return-local forms, C++03
  conditional return forms, and six historical optimization contexts;
  the first positive original-sized result is **131/193**.
- `evaluate_assemble_binop_alignment_oct14.py`: genuine local
  `int`/`unsigned`/`const`/type-pointer ownership, else-if vs independent
  comparisons, selective period GCC branch predictions, label/jump/loop
  alignment. Best is **162/193** with only the case-1 unlikely control.
- `evaluate_assemble_binop_probability_oct14.py`: bounded combinations
  of the four branch likelihoods on the original dispatch topology. None
  changes the best **162/193**, and most give incorrect size or layout.

The original `GetOpType`, `MLGameString` copy and integer/float specialist
calls are *named* authentic ELF symbols and relocation-verified;
no assumption about their internal source is necessary for this dispatcher.

## Independent 119-byte AST statement-list translator control

Target: `Translate(ASTNodeList<ASTStmt*>&)` at `0x0806ada0`.
The original loops over a 12-byte list, ignores null translated
statements, initializes the accumulator with the first non-null statement,
and for subsequent elements allocates a 16-byte virtual `IR::Seq_Data`
containing type tag 1, vtable at `0x080760e0`, old accumulator and
new item (in that order).

Existing `vuc/probes/translate_stmt_list.cpp` models this genuine
semantic graph. The initial candidate emits 116 bytes against the original
119. `evaluate_translate_stmt_layout_oct14.py` extends the prior
source-form controls to distinct early-continue paths, conditional
sequencing, outer-loop snapshot/index lifetime and historical 2004
jump/label/loop alignment with/without `-frename-registers`.
These 720 original-era builds did not produce any improved **accepted**
original-length byte match. A small size coincidence is not sufficient:
the source still needs the original stack-lifetime and block arrangement.
No partial is promoted from this target.

## Next targets

1. Before revisiting `AssembleBinOp`, recover genuine **IR2Asm.cpp**
   predecessor/translation-unit context and branch probabilities from
   original emitted code. Stop revisiting simple equivalent four-if
   permutations.
2. Explore `Canonize_Reduce(IR::Fragment_Data*)` at `0x08050460`,
   224 bytes; original disassembly shows a hash-node insertion path for
   fragment kind 1 and an environment-clear / stmt-reduce path for kind 2.
3. Explore `Translate(ASTNodeList<ASTExpr*>&)` at `0x0806ae20`,
   256 bytes, using recovered list and IR object layout rather than
   assuming its source matches the statement-list folding path.

No source in `vuc/src/`, accepted function manifest, original ELF or
root VCL reconstruction was modified. Accepted coverage remains
**195 EXACT, 15 EXPERIMENTAL, 88 UNRESOLVED** and **13,992** exact
instruction bytes; the new 162-byte partial is an *independent* diagnostic
and is not added to those counts. Whole linked ELF byte equality remains
unverified.

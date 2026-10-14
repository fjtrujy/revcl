# VUC October 14: bounded small-function negative controls

Branch: `vuc`. Starting HEAD: `d2ddfa6`.
No additional exact function was obtained in this batch. These tests
are source-level controls for the **original** VUC Linux x86 ELF,
compiled with the January 25, 2004 GCC 3.3.3 prerelease and GNU as 2.15
wrapper in the existing `revcl-oracle` Lima VM. No original archive,
ELF byte, assembler instruction or accepted source was changed.

The previous successful batch accepted
`MLGameStringTable::Add(const char*, bool)` at **263/263** using a
named C++03 `const char *const &` reference. We tested whether related
real C++ source lifetimes could transfer to shorter outstanding helpers,
rather than assuming this local GCC behavior was a universal solution.

## Bounded experiments

| Function | Original size | Observed best exact-length score | New evidence |
| --- | ---: | ---: | --- |
| `AsmOutput_WritePrologue(std::ofstream&)` `0x08049b40` | 30 | **22/30** | 156 relocation-checked objects, 12 source forms × 13 historical flags; same compiler argument-register/load-order residual |
| `MLTempString::MLTempString()` C2 `0x08071c20` | 101 | **95/101** | 14 genuine constructor forms, with/without formatted-constructor predecessor, across 11 optimizer contexts; same six register-choice offsets |
| `IsAccumulatorable(IR::Operator_Data*)` `0x0805a570` | 77 | **61/77** | 10 ordinary nested-if/switch/goto/return-control-flow bodies, 7 branch-likelihood hypotheses, 6 alignment controls; case-12/true-block pad placement remains wrong |
| `MLHashTable<MLGameString,SymbolTableEntry*,...,513>` copy constructor `0x0804f420` | 166 | **No new exact-length candidate** | 8 index representations, 7 real node/tail reference forms, 5 flags; the explicit volatile source index inflates compiled length rather than matching stack lifetime |

The prologue partials were scored independently using the strict
`match_probe.py` original-relocation checker. All **156** tested
functions were 30 bytes, and the best remained **22/30**. Some of the
literal-address and stream-argument loads use opposite registers/order
from the original; simple C++ aliases do not restore the historical
scheduling. The original 92-byte prologue literal is mapped at
`0x080720a0`, not invented.

The MLTempString C2 independent baseline reproduces **95/101** with
exact byte differences at relative offsets **22, 28, 55, 60, 66,
70**. The relevant operations are the FormatString empty-string
address and the singleton's memset length/value and guard
initialization, rather than missing API-level behavior. The accepted
**102/102 C1 constructor** was left unchanged.

For `IsAccumulatorable`, original GCC places a four-byte alignment
before the kind-12 block at offset **+0x20**, followed by seven bytes
before the shared true block at **+0x30**. Natural source normally
places all eleven alignment bytes before the true block. Rewriting
legitimate success/failure labels and predicting the actual child
operand check did **not** improve the original 61/77 score. This is
basic-block probability/emission-context evidence, not a reason to
insert manual NOPs.

For the MLHashTable copy constructor, the original stores the
513-bucket outer index in a stack home; the current natural source
uses register induction and produces a **162-byte** body. An
ordinary volatile index forces stack storage, but the tested
variants grow to larger function sizes (often 194 or 200 bytes).
That is a failed source-lifetime hypothesis: **a semantic stack
location is not sufficient** without matching real indexed
array access and compiler register allocation. No produced
166-byte candidate was promoted.

## Reproduction

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_prologue_sources_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_ml_temp_c2_source_lifetimes_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_accumulatorable_source_labels_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_hash_copy_ctor_index_oct14.py"
```

Compiler objects, generated temporary .cpp files, and individual
comparison JSON files live in `/tmp/vuc-binutils215` **inside the
Lima VM**. Only reproducible evaluator scripts and this evidence are
tracked. The prologue evaluator's default is bounded to the 156
isolated objects already measured; existing 2004-style source-unit
controls live in `evaluate_asm_real_unit_neighbors_oct13.py`.

## Next sources worth reconstructing

Recover the original source-unit emission context rather than
revisiting equivalent C++ spelling:

- `Instruction` arena destructor `__tcf_1` remains **45/46**,
  apparently a postreload scratch-pop register divergence; inspect
  true original `Instruction.cpp` predecessor order/RTL.
- `MLTempString` C2 and prologue mismatches are register
  choice/scheduling issues despite matching original lengths.
- The hash copy constructor needs the actual outer-index carrier
  and/or original `MLHashTable` class-template expression structure.
- For earlier unresolved AST template lists, recover genuine
  `ASTNodeList::Add()` before resuming repeated alignment sweeps.

No acceptance manifest was changed. The authoritative VUC inventory
remains **196 EXACT, 15 EXPERIMENTAL, 87 UNRESOLVED**, with
**14,255** independently verified instruction bytes. Whole-ELF
byte identity is not established.

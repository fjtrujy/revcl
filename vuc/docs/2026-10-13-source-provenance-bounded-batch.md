# Bounded historical VUC source-provenance controls (October 13, 2026)

Branch: `vuc`. All experiments use the original ZIP-backed ELF as their
oracle and ordinary C or C++03 source compiled by the recovered January 25,
2004 GCC 3.3.3 + GNU as 2.15 wrapper in `revcl-oracle`.

## Source state and safeguards

The checkout began at `817b46c`, tracking `origin/vuc`; both local
`main` and fetched `origin/main` were at `cd35047`. No additional
rebase was warranted. The 133 pre-existing untracked paths were preserved.
The original archive, original executable, accepted `vuc/src/` functions,
and strict exact/data/BSS candidate manifests were not modified.

These are **negative controls and authentic structural findings**, not new
byte-exact function acceptances. None authorizes copying original bytes,
inserting assembler or padding, forging relocations, or inferring complete ELF
identity.

## AsmOutput predecessors and AlignString

`vuc/probes/evaluate_asm_predecessor_context_oct13.py` runs **64**
compilations: four normal C++ iostream insertion forms for each of the two
real source-unit predecessors (`WritePrologue`, `WriteEpilogue`) under
four historical optimizer configurations. The source under investigation
is the genuine combined `vuc/src/AsmOutputUnit.cpp`, including
`AlignString`, `AlignConstant`, and `TabConstants`.

Every scored function authenticates its relocations and all **127 original
source-unit literal bytes**. The best remains:

| Function | Original bytes | Best instruction match | Status |
| --- | ---: | ---: | --- |
| `AsmOutput_WritePrologue` | 30 | 22/30 | experimental |
| `AsmOutput_WriteEpilogue`, in this unit | 30 | 22/30 | its independent accepted object remains 30/30 |
| `AlignString` | 145 | **143/145** | experimental, offsets 41/47 |
| `AlignConstant` | 69 | **69/69** | already accepted |
| `AsmOutput_TabConstants` | 127 | **127/127** | already accepted |

Changing the two genuine stream-insertion expressions does not select the
original EDX instead of EAX for `strchr`'s character argument. The
`-fno-regmove` control reduces `TabConstants` to 105/127 and must not
replace the accepted flags. No artificial predecessor was added.

## Printer AST inheritance and register controls

The original RTTI and existing `vuc/src/ASTRttiNames.cpp` identify
`ASTFunCall : ASTNode`. The original call printer at `0x0805b770` reads
its callee from offset `+8` and its argument list from `+12`, making
an ordinary polymorphic `ASTNode` base a plausible source reconstruction.

`vuc/probes/evaluate_printer_ast_layout_oct13.py` checks seven
flat/inherited/virtual-method or destructor declarations with four
optimization combinations; all retain the actual offsets and nine
original source-unit literal bytes. Inheritance alone does not change
the historical code: plain `-O2` matches **96/98** printer bytes, but
the neighboring `Printer_UglyPrint` is only 33/40. With
`-frename-registers`, the call printer remains **92/98**, while
`Printer_UglyPrint` stays **40/40** exact. The two plain-mode residual
bytes are the EAX-versus-ECX load/store pair at offsets 23/29.

`vuc/probes/evaluate_printer_register_flags_oct13.py` tests 27 named
historically plausible instruction-selection and alignment controls.
None resolves the remaining register pair while maintaining the accepted
printer neighbor. Changing `-march` to i386/i486/i586 changes the
function length, and disabling the second scheduler degrades the score.

## Authenticated IR hierarchy and statement translator

The original RTTI inheritance relationship recovered by
`vuc/src/IRRttiNames.cpp` and `vuc/src/IRVtableProbes.cpp` is
**`IR::Seq_Data : IR::Stmt_Data`**, not
`Seq_Data : IR::Data` as guessed by the earlier standalone
`vuc/probes/translate_stmt_list.cpp`. `Stmt_Data` is a root in that
authenticated RTTI structure. The original `Translate(ASTNodeList<ASTStmt*>&)`
at `0x0806ada0` is 119 bytes; it stores the result pointer in
`[ebp-0x10]`, advances the list in ESI, and initializes the allocated
sequence node's kind, vtable and both child pointers.

Two evaluators preserve the original 119-byte target, real relocations
and period C++ source:

- `evaluate_translate_seq_shapes_oct13.py` checks seven ordinary
  `Seq_Data` initialization orders with five natural loop forms across
  four flag sets. The successful compilations remain 116, 108 or 104
  bytes, so none is eligible for an exact byte comparison.
- `evaluate_translate_rtti_hierarchy_oct13.py` corrects the inferred
  single-inheritance relationship and checks six plausible polymorphic
  `Stmt_Data` definitions and three loop forms across four flag
  sets. The nearest body remains 116 bytes.

The **source model** has improved through genuine RTTI evidence, but
no function-level matching-byte improvement is claimed. Future work
should recover the original live ranges/constructor emission and
translation-unit context rather than manually storing into the original
stack slot or hardcoding field writes to mimic the disassembly.
A separate volatile-result control emitted 132 bytes and did not help.

## glibc csu initializer and historical architecture flags

The original `__libc_csu_init` is **84 bytes** with saved EDI/ESI/EBX
pushes and an `__i686.get_pc_thunk.bx`-based PIC relocation sequence.
The previously accepted `__libc_csu_fini` **80/80** is preserved
independently. The original authenticated GOT and init-array symbol
addresses remain required.

`vuc/probes/evaluate_libc_csu_prologue_oct13.py` tests ordinary C
startup expressions from the established two source fragments under
optimization, code scheduling, preferred-stack-boundary and i386 target
options. `-O1 -fPIC` can produce the original 84-byte *length* but only
**7/84** instruction bytes; the `-O2` default is 94 bytes,
`-march=i386` 74 and `-march=i486` 78 in this source.
The historical compiler rejects modern `-mtune` and `-fno-web`;
they have been excluded from the reproducible follow-up script.

`vuc/probes/evaluate_libc_csu_architecture_oct13.py` tests **96**
combinations of real i386/i486/i586 tuning, loop/jump alignment,
and register renaming with the authenticated PIC matcher. **None emits
84 bytes**, so no relocation-scored new startup function is accepted.
These experiments distinguish compiler code-generation/prologue behavior
from any claim that the current C source exactly matches glibc 2.3.2.

## Reproduction

Run from the existing Mac checkout using the Linux historical compiler:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_asm_predecessor_context_oct13.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_printer_ast_layout_oct13.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_printer_register_flags_oct13.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_translate_seq_shapes_oct13.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_translate_rtti_hierarchy_oct13.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_libc_csu_prologue_oct13.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_libc_csu_architecture_oct13.py"
```

All intermediate objects and generated C++ variations stay under
`/tmp/vuc-binutils215` in the Linux VM. Exact acceptance continues
to be controlled only by `vuc/config/exact_candidates.json`, and
whole-ELF equality remains unachieved. The stable baseline is **188
exact/298**, **12,702 original instruction bytes**, **244 exact
initialized-data objects/71,536 bytes**, and **12 BSS declarations**.
The original function ledger remains **188 EXACT, 12 EXPERIMENTAL,
98 UNRESOLVED**. Current experiment results must not be promoted.

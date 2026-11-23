# VUC 2004.1 — historical GCC source-unit and CPU controls

**2026-11-23**, long-running `vuc` branch. Original SHA-256-verified
VUC 2004.1 ELF32 remains unchanged. No original binary, vendor ZIP,
historical GCC executable or generated object was added to Git.

This ledger records *negative* source-level hypotheses tested before
the [two new exact MLTempString C2 constructor matches](2026-11-23-mltemp-dual-c2-byte-exact.md).
All experiment scripts operate on a scratch copy, use the recovered GCC
3.3.3 Jan 25/Dec 6 + GNU as 2.15 toolchains and score only real
original-sized, relocation/literal-authenticated function bodies.

| Experiment | Compiler runs | Verified result |
| --- | ---: | --- |
| `evaluate_script_unit_order_20261123.py` | 60 | Genuine lexer, main, error and initializer TU permutations: best `script_error` **155/161**; 24 accepted-size unchanged, 6 wrong-sized, 30 compiler errors |
| `evaluate_symbol_enter_eh_alignment_20261123.py` | 52 | GCC branch/exception layout controls: only `-fno-reorder-blocks` reached **185 original bytes**, but just **38/185** matched and broke exact siblings; no gain |
| `evaluate_symbol_enter_source_eh_20261123.py` | 70 | Authentic empty-scope, hash node, bucket and link-store source forms: no original-length exact source, no gain |
| `evaluate_prologue_reverse_args_20261123.py` | 32 | Inline ostream helper parameter order: all **22/30** on original-length function |
| `evaluate_value_scalar_union_20261123.py` | 40 | Distinct scalar-float union and vector lane-0 members: thirty preserved **203/219**, remainder wrong-sized or invalid |
| `evaluate_instruction_real_inline_predecessors_20261123.py` | 68 | Real static-inline predecessor and translated TU contexts: 64 valid **45/46** arena cleanup, vector neighbor **265/265**; four compile errors |
| `evaluate_tryopen_source_lifetimes_20261123.py` | 46 | Real `std::ifstream` construction / return / inline helpers: best **650/754**, wrong-size, wrong-relocation or regressed alternates |
| `evaluate_historical_i386_cpu_targets_20261123.py` | 138 | Source-compatible GCC CPU scheduling of `TryOpenFile`, `Value::operator+` and `SymbolTable::Enter`: no admissible gain |
| `evaluate_near_exact_i386_cpu_20261123.py` | 84 | i386 CPU scheduling of **96/98** `Print(ASTFunCall*)` and **155/161** `script_error`: unchanged best |
| `evaluate_scope_vector_cpu_scheduling_20261123.py` | 46 | CPU model as the cause of the 13 `sched2` growth-store mismatches: **733/746** on eligible builds, no gain |

In total **636 negative compiler trials** (the 144-experiment
MLTempString source-order matrix is documented separately). These
measure attempted compiler outputs, not individual new coverage bytes.

## The positive discriminator

The shared constructive insight is that **real historical method
definition order**, rather than source-level aliases or CPU tuning,
affects GCC's register allocation. The additional
`evaluate_mltemp_real_tu_20261123.py` test ran **144 compiler recipes**:
all six method orders of three real member definitions, three ordinary
Set return contracts, four compiler flag configurations, and both
historical GCC snapshots.

Two different genuine owner orders recovered the original C2 constructors
**101/101** and **110/110** with all relocations; their respective C1
siblings were simultaneously exact. These two historical-only owner
sources are now accepted under `vuc/src/`. Other methods in those
provisional source owners are not necessarily exact, and the owners
are deliberately **not** linked into one binary.

## Independent regression after promotion

- **209/209** exact named functions, **16,934/16,934** verified bytes,
  GCC Jan25 and independently GCC Dec06; original-sized bodies and
  authentic relocations.
- **34 of 35** selected nonexact experiments retain original-length
  verified partials, **6,990/10,923** exact original-offset bytes
  within those 34 functions; **zero regressions**.
- **244/244** initialized data objects, **71,536/71,536** bytes.
- **12/12** BSS declarations, **18,437** source allocation bytes.
- **206/206** host Python unit tests, and **14/14** CMake native tests.
- Full reconstructed VUC ELF does **not** exist: byte-exact function
  matching is not linked whole-file matching.

All result ledgers and generated historical object files stay in the
Linux guest at `/tmp/vuc-*-20261123/`, while scripts, source candidates
and the complete updated function-status inventory are committed on
`vuc`. The earlier hypotheses remain reproducible; do not repeat
them without new causal compiler or source-unit evidence.

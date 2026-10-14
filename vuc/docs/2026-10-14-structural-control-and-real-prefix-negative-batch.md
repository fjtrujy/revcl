# VUC: structural control flow and real predecessor batch

Date: 2026-10-14. Branch: `vuc`.
Historical target: VUC 2004.1 Linux i386, read from the archived upstream ZIP.
All experimental objects were built inside `revcl-oracle` with the recovered
January 25, 2004 GCC 3.3.3 prerelease and GNU as 2.15 wrapper.

## Git and acceptance checkpoint

Fetched `origin/main` at `9905121` and replayed 108 VUC commits without
conflicts, from `98496d1` to `29f1ab0`. All 146 pre-existing untracked
paths remained untouched and the tracked tree was clean before the batch.

After rebasing:

- Host suite: 148/148 tests pass.
- Strict historical function gate: 195/195 functions, 13,992/13,992
  accepted instruction bytes.
- Separate initialized-data gate: 244/244 named objects,
  71,536/71,536 verified bytes.
- Separate BSS gate: 12/12 declarations, 18,437 allocation bytes;
  these represent zero file-backed bytes.

None of these tests establishes whole-linked-ELF identity.

## Experimental result: no new exact match or improved partial

All measured original-length scores below use `vuc/scripts/match_probe.py`,
which checks original bytes and authentic named/relocation targets. Compiler
objects and generated source variants are kept under `/tmp/vuc-binutils215/`.
Only the reusable experiment drivers are committed.

| Original function | Address | Original length | Previous best | New best | Outcome |
| --- | --- | ---: | ---: | ---: | --- |
| `Value::operator+()` | `0x0806e9b0` | 219 | 172 | **172/219** | Unchanged |
| `Value::operator/()` | `0x0806ec50` | 235 | 178 | **178/235** | Unchanged |
| `yyFlexLexer::yy_get_next_buffer()` | `0x080611c0` | 550 | 430 | **430/550** | Unchanged |
| `Print(ASTFunCall*, ostream&)` | `0x0805b770` | 98 | 96 | **96/98** | Unchanged |
| `MLTempString::Set(char const*, ...)` | `0x08071d00` | 117 | 111 | **111/117** | Unchanged |

The original `Instruction` arena destructor remains **45/46** and was not
retested in this batch: the prior two-snapshot RTL and 25 original-method-order
negative controls already delimit it to one dead cleanup-register byte.

### Value addition: genuine returns and switch topology

Script: `vuc/probes/evaluate_structural_control_flow_oct14.py`.

Nine source variants under three historical optimization/alignment settings
(27 builds) tested the natural switch, early conversion-failure returns, a
default-case return, per-arm returns, shared `goto` epilogue, and three lexical
orders of the same integer/scalar/vector arithmetic cases. These are valid
C++03 control-flow explanations, not injected padding.

Every original-length `-falign-jumps=4` version compiled to the **same
172/219** original byte score, with the same first recorded difference at
relative `+0x7f`. `-falign-jumps=8` produces 227 bytes rather than 219.
This demonstrates that GCC 3.3.3 canonicalizes these particular return and
switch spellings. It does **not** settle the original branch-probability
asymmetry: the original vector arm begins at `+0xa7` without the padding seen
in the compiled j4 case, while the original scalar arm begins at `+0xd0`.
Further progress needs a different predicate/edge or true compiler processing
context, not more lexical case shuffling.

### Value division: x87 expression lifetime

The same script tests eight vector-case hypotheses, each under three
historical alignment choices (24 builds). They include reciprocal
multiplication, direct division, per-component local reciprocals,
denominators, numerators, all reciprocals scoped before stores, and genuine
array induction loops.

Per-element locals for numerator, denominator and reciprocal leave the emitted
original-length function code effectively identical to the best baseline:
**178/235** under `-falign-jumps=8`. Direct division is only 227 bytes,
not the original 235. Hoisting all reciprocals gives 250-259 bytes; loops are
not original-length candidates. The earliest mismatching byte for the
original-length variants remains `+0x7f`, followed by the vector arithmetic
region beginning near `+0xa6`. This does not authenticate the source's
scalar/x87 evaluation order, but it rules out several ordinary temporary
lifetimes as the missing explanation.

### Flex buffer: copy induction, ownership and EOF paths

The same script tests ten structural forms under two historical alignments
(20 builds). It changes the byte-copy loop among indexed, countdown and
pointer-end forms; tests a saved buffer owner outside the growth loop and a
reference alias; and reexpresses no-fill/EOF return paths.

At `-falign-jumps=1`, the baseline and noncopy EOF/alias expressions
still emit **430/550** byte-identical candidate results. Pointer/countdown
copies shift the function to 551 bytes; j4 usually emits 552 bytes.
The first recorded mismatch in original-length cases remains at `+0x37`,
and the next contiguous mismatches begin at `+0x4b`. A switch in expression
spelling alone will not fix the input-buffer growth liveness or aliases; next
work should compare actual GCC 3.3 RTL induction and global allocation
against the original load/store graph.

### Printer: ordinary inlined AST getters

Script: `vuc/probes/evaluate_printer_inline_getter_oct14.py`.

Ten real source variants under `-O2` and `-O2 -frename-registers`
(20 builds) use direct field access, inline const/nonconst callee getters,
a callee-reference getter, direct and pointer-style argument getters, and
free inline accessors. Every candidate uses actual `ASTFunCall` fields and
the authenticated `Print` call/literal relocations.

Plain argument getters remain **96/98**; their mismatches are unchanged at
offsets `+23` and `+29`. Callee getters instead alter earlier useful
allocation and regress to 79/98. `-frename-registers` leaves the best
variants at 92/98. The missing ECX receiver load is not produced merely by
introducing a natural inline getter.

### MLTempString::Set: authentic methods as GCC predecessors

Script: `vuc/probes/evaluate_mltemp_real_prefix_oct14.py`.

A separate final experiment tests 16 original-method combinations (32 builds)
from real `MLTempString` formatted/default constructors,
`MLTempStringData::FormatString` and the varargs `Set` method, each with
`-O2` and `-O2 -frename-registers`. No artificial compiler-state functions
or hand-crafted registers were used.

The isolated `Set` remains **111/117**. Placing the default constructor or
real `FormatString` before it leaves all six differences unchanged at
relative offsets **32, 42, 71, 76, 82, 86**. Adding the formatted constructor
changes additional bytes and worsens the score to **105/117**. Explicit
register renaming produces 118 bytes instead of the original 117.

Disassembly identifies the six discrepancies as **register selections**, not
different arithmetic or resolved relocation targets: the original loads the
format argument through ECX instead of candidate EAX, and its initializer
constants use EDX/EAX instead of candidate ECX/EDX. Real predecessor inclusion
changes compiler output, but these predecessors do not recover the original
processing state. The VCL peephole2 lesson remains plausible but cannot be
claimed as a proven VUC explanation without a specific emitting owner.

## Reproduction

Run on the existing Mac, with the historical Linux VM present:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_structural_control_flow_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_printer_inline_getter_oct14.py"
limactl shell revcl-oracle -- python3 "$R/probes/evaluate_mltemp_real_prefix_oct14.py"
```

All scripts use the genuine archived-symbol relocation comparator. They do not
modify accepted production source, manifests, compiled objects or the original
binary. Scores from functions with incorrect emitted length are **not**
promoted, even when semantics are plausible.

## Next experiments justified by this batch

1. **Value::operator+**: inspect a particular original switch edge's GCC 3.3
   `bbro` probability / `peephole2` alignment in a real source-unit context;
   lexical return rewrites have now been eliminated.
2. **Instruction arena `__tcf_1`**: identify the *actual* original
   processing predecessor that could advance GCC 3.3's scratch selection to
   ECX. Avoid reordering the same already-tested methods.
3. **MLTempString::Set**: investigate genuine ownership/emission of the
   formatted C1/C2 constructors and `MLTempStringData::Get` initializer
   before trying any scratch-selection hypothesis.
4. **Flex growth**: recover the owner/pointer induction graph and first
   mismatching RTL stage instead of trying cosmetic byte-copy loops.

No acceptance manifest is changed: **195 EXACT / 15 EXPERIMENTAL /
88 UNRESOLVED**, with no claim of whole-linked-ELF byte identity.

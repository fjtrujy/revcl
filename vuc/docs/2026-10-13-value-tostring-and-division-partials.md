# VUC Value ToString and division source reconstruction batch

Date: 2026-10-13; branch `vuc`; historical GCC 3.3.3 (2004-01-25
prerelease) and GNU as 2.15 in the existing `revcl-oracle` VM.

## Scope and acceptance

This batch started from `4e31d95`, the exact
`Value::TypeConvert()` milestone. The original ZIP and ELF were
inspected read-only. There were no outstanding tracked modifications
and 146 pre-existing untracked paths were preserved.

**No newly exact function was recovered.** Two genuinely reconstructed
C++ methods improved from previously incomplete/unknown function
coverage. They are now **experimental** in
`vuc/config/selected_candidates.json`; neither enters
`vuc/config/exact_candidates.json`.

| Original VUC function | Original address | Verified partial | Status |
| --- | --- | ---: | --- |
| `Value::ToString() const` | `0x0806f6f0` | **159/221** | Experimental |
| `Value::operator/(Value const&) const` | `0x0806ec50` | **178/235** | Experimental |

The unchanged exact baseline is **190/298 original named functions** and
**13,144/13,144 accepted instruction bytes**, with 244/244 initialized-data
objects (71,536 bytes) and 12 BSS declarations. After accepting the two
new experimental statuses, the inventory is **190 EXACT / 14
EXPERIMENTAL / 94 UNRESOLVED**. Function-level exactness is separate
from whole-ELF identity, which has not been established.

## Value::ToString(): original length and full literal authentication

Source: `vuc/probes/ValueToStringUnit.cpp`.
Probe: `vuc/config/value_to_string_oct13_probe.json`.

This ordinary C++03 reconstruction initializes `char buffer[64] = ""`,
uses a genuine switch on the original Value type and formats integer,
scalar float, or four floats with `sprintf`, then returns an
`MLGameString(buffer, false)`. These semantics explain the original
32-bit struct-return ABI, C++ constructor call and x87 float-to-double
argument promotion. No assembly or synthetic compiler helpers are
included.

Original ELF addresses and exact literal bytes:

| Candidate initialized data | Size | Original virtual address |
| --- | ---: | --- |
| 64 zero bytes for `char[64]` initialization | 64 | `0x0807669c` |
| `"%d"` | 3 | `0x08072971` |
| `"%f,%f,%f,%f"` | 12 | `0x08076690` |
| `"%f"` | 3 | `0x08076699` |

All 82 bytes were checked directly against the ZIP-backed original
ELF, not guessed based on neighboring source. The
`relocation_literals` manifest authenticates these **discontiguous
original addresses** without pretending they were one continuous
original string region. All **eight** text relocations are resolved,
including `sprintf@plt` (`0x080499bc`),
`MLGameString::MLGameString(char const*, bool)` (`0x08071720`) and
the four verified literal-address references.

Historical compilation with
`-O2 -frename-registers -falign-jumps=4` emits the correct **221-byte
function length**. An isolated authentic implementation first matched
**145/221**. Restoring genuine
`Value::operator+()`, `Value::operator-()`, and
`Value::operator*()` as immediate source-unit predecessors improved
matching to **159/221**, with all eight relocations and all literals
still correct.

The residual mismatches start at offset 65 and span the case-4 block
(offsets 131 through 191). The candidate GCC output places its case-4
branch at relative `+0x84`, while the original starts at
`+0x83`; its case-3 target at `+0xc0` remains aligned. The
register, operand preparation and branch-placement differences have
not been recovered.

Controls:

- `evaluate_value_to_string_forms_oct13.py`: all six genuine
  lexical switch orders, six scalar/vector expression-local forms,
  and historical jump alignment variants. None generated an exact
  function.
- `evaluate_value_to_string_real_context_oct13.py`: real arithmetic
  source unit (159/221), comparison source unit (158/221), and a
  genuine subsequent `TypeConvert()` source (159/221).
- `evaluate_value_tostring_neighbor_families_oct13.py`: progressive
  inclusion of original-order reconstructed division, comparisons,
  signed relational operators, logical operators and bitwise
  operators. All successful same-length normal-optimization variants
  remained at 159/221.
- `evaluate_value_to_string_alignment_oct13.py`: **144** combinations
  of `-falign-jumps`, `-falign-labels`, and `-falign-loops`. None
  improved 159/221 or reproduced the original branch layout.

Do not hardcode the original NOP bytes or alter the original ELF.
Future work should inspect the original C++ switch dispatch, GCC
basic-block profiling/alignment and source-unit context, rather than
adding cosmetic padding.

## Value division: local denominator improved a partial

Source: `vuc/probes/ValueDivisionUnit.cpp`.
Probe: `vuc/config/value_division_partial_oct13.json`.

The source preserves the original +, -, * arithmetic definitions before
the genuine division method, retains signed integer division, scalar
float division, and vector reciprocal-multiply operations, and tests
ordinary C++ lifetimes of scalar operands.

`evaluate_value_division_scalar_forms_oct13.py` explores ten plausible
scalar arithmetic expressions under five historical compiler flag
sets. Introducing a block-scoped local
`float divisor = temporary.values[0];` before dividing
`result.values[0]` improves the original-sized function from
**174/235** to **178/235** instruction bytes, using
`-O2 -frename-registers -falign-jumps=8`. Its single
`Value::TypeConvert` relocation resolves to the original
`0x0806f7d0`. The vector float calculation still differs in
the original's x87 operation scheduling and some branch alignments.
There is no initialized literal data in this method.

The source is partial; no claim is made about original lexical
variable names or whole-translation-unit provenance.

## Reproduction and safeguards

Run from the checkout, using the historical compiler:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- /bin/sh -c '
  R=/path/to/revcl/vuc
  C=/tmp/vuc-binutils215/g++-jan25-as215
  "$C" -O2 -frename-registers -falign-jumps=4 \
    -c "$R/probes/ValueToStringUnit.cpp" \
    -o /tmp/vuc-binutils215/tostring-verified.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/tostring-verified.o \
    --config "$R/config/value_to_string_oct13_probe.json"
  "$C" -O2 -frename-registers -falign-jumps=8 \
    -c "$R/probes/ValueDivisionUnit.cpp" \
    -o /tmp/vuc-binutils215/division-verified.o
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/division-verified.o \
    --config "$R/config/value_division_partial_oct13.json"
'
```

The reproducible source-order/flag evaluators live under
`vuc/probes/`. Scratch C++ variations and objects are generated only
under `/tmp/vuc-binutils215` inside the Linux VM, and the original
archive and executable remain unchanged. These selected partial
candidates intentionally fail the strict byte-exact requirement and
**must never be added to the exact manifest**.

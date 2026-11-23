# Scheduler CSE flag investigation

This research follows [Enum-typed fields and compiler flags](ENUM_TYPES_AND_COMPILER_FLAGS.md).
The objective is to replace the historical GCC 3.2.2 file-specific
`-fno-cse-follow-jumps` options for `src/wrangle.cpp` and
`src/wrangleCore.cpp` with natural C++98 source that independently produces
the byte-exact reference stripped ELF, **without assembly, data patching,
compiler-specific section hacks, or changes to other translation units**.

## Outcome

**No acceptable source-level replacement has been found.** Retain both
exceptions in `config/build.json` until a candidate reproduces the entire
reference hash:

`83bee75205b5b0f37dbefc2c2769eba2c353befa532472c49aedb2ebaf168274`

All temporary source probes were restored; no unsuccessful source changes
should be merged. Experiment outputs/scripts are kept under ignored
`tmp/` in the existing checkout for further investigation.

## Exactly what differs

The exceptional flag changes only **four instruction bytes in each
translation unit**, excluding address/displacement shifts caused by that
difference:

| Translation unit | Exact reference `.text` size | Without the flag | Origin |
| --- | ---: | ---: | --- |
| `wrangle.cpp` | `0x12ba` (4,794) | `0x12b6` (4,790) | `wrangleMode1Worker` |
| `wrangleCore.cpp` | `0x11b6` (4,534) | `0x11b2` (4,530) | `wrangleMode0Worker` |

Both operations are the full-loop `drag(table, instruction, &pos,
table->loopSize, 0)` call, paired with the half-loop
`drag(table, instruction, &pos, table->loopSize >> 1, 0)` call.
For the mode-0 worker, the local instruction pointer is named
`dragInst`.

### Mode 1: the two bytecode forms

With `-fno-cse-follow-jumps`, the full-loop path emits:

```text
ae4: subl  $0xc, %esp
ae7: pushl $0x0
ae9: pushl 0x38(%ebx)       ; loopSize, 3 bytes
aec: jmp   0x7bd             ; join AFTER the shared push
```

Without the flag, the same path emits:

```text
ae4: subl  $0xc, %esp
ae7: pushl $0x0
ae9: jmp   0x7bc             ; join AT pushl %eax
```

The half-loop path has already loaded `table->loopSize` to `%eax`.
The shared code at `0x7bc` contains `pushl %eax` (one byte), followed
by the remaining `drag` arguments. **Both forms pass the full-loop
argument**. The optimized path has one fewer redundant memory reload,
not an absent function parameter or a demonstrated correctness bug.

Mode 0 follows the same pattern: with the flag, the full-loop path
has `pushl 0x38(%ebx)` followed by a jump to `0x323`; without it, it
jumps directly to the common `pushl %eax` at `0x322`.

### GCC pass responsible

`g++ -da` on the pinned RH9 compiler produces RTL dumps. The key
change is already visible in `wrangle.cpp.08.cse` (after RTL
`03.jump`); the original RTL `1265` full-loop argument is a memory
read from `table->loopSize` in both builds:

```text
(set (mem (pre_dec esp)) (mem (plus table 56)))
```

With the exception flag, it remains the memory read. Under normal
`-fcse-follow-jumps`, GCC's first CSE pass replaces that value with
pseudo-register 186, already loaded during the preceding nonzero
`loopSize` test:

```text
(set (mem (pre_dec esp)) (reg:SI 186))
```

Register allocation places that pseudo-register in `%eax` and
later block ordering folds the full-loop push into the same shared
`pushl %eax` instruction used by the half-loop path. **This is a
causal compiler data-flow substitution, not a formatting issue.**

The reference instructions are still emitted by this compiler if
the original per-file optimization exception is present.

## Negative experiments

All experiments compiled against the **same pinned GCC 3.2.2**
without the per-file exception, retaining the common
`-O2 -frename-registers -fmerge-all-constants` flags and the original
link-input order. Candidates with an unchanged text length were also
linked, stripped and compared to the reference hash. Matching `.text`
size alone is insufficient.

Several batches explored the two functions separately:

| Approach | Result |
| --- | --- |
| Substitute `lsz` or other cached locals for full-loop `loopSize` | Mode 1 can regain four bytes, but emits `pushl -0x70(%ebp)` instead of `pushl 0x38(%ebx)` |
| Make the full-loop argument volatile | Regains text size but emits `movl 0x38(%ebx), %eax` and reuses `pushl %eax`; bytes differ |
| Cast through references, integer pointers, `long` or GNU `may_alias` types | CSE usually recognizes the original access; no exact match |
| Change enclosing `loopSize` guard to a volatile read | Breaks CSE reuse but adds a second memory load in the half-loop path |
| Capture a volatile or nonvolatile `loopSize` snapshot for the guard and half path | May preserve size, but changes register allocation, memory loads or stack slots |
| Reverse `if/else`, add explicit `goto` joins, use `switch`/`do while` or a ternary call | Normal optimization often reproduces the same shortened path; other shapes change layout |
| Use inline getter, template getter, references or pointer-to-member access | Compiler folds these into the same expression and removes four bytes |
| Vary signedness, nonvolatile casts, Boolean tests, volatile operand-category reads | Either identical optimized output or additional instructions |
| Apply `__builtin_expect` hints to either branch | Changes block layout or remains optimized; no exact match |

The ignored scratch files `tmp/wrangle-cse-focused.py`,
`tmp/wrangle-cse-variants.py`, `tmp/cse-guard-probes.py`,
`tmp/cse-cfg-probes.py`, `tmp/cse-snapshot-probes.py`,
`tmp/cse-alias-probes.py`, `tmp/cse-getter-probes.py`, and
`tmp/cse-inner-probes.py` contain the individual temporary trials.
The RTL dump artifacts are under `tmp/cse-report/`.

Some trial forms deliberately used unsafe or unnatural aliases solely
to reject hypotheses; **none are proposed as production source**.
Especially avoid replacing file-specific flags with volatile casts,
inline assembly, synthetic no-op barriers or nonportable branch
restructuring just to coax historical GCC's optimizer.

## Remaining promising direction

To remove either flag, the reconstructed source must change the
**actual GCC first-CSE value equivalence**, yet preserve the loaded
`loopSize` for the half-loop case, all original argument pushes,
register assignments and branch layout. Merely changing an expression's
spelling does not affect the relevant equivalence graph.

A credible future candidate would come from reconstructing the
surrounding **value lifetime, control-flow dominance and member
alias/mutation semantics** of `loopSize` in the full scheduler worker
functions, then evaluating the `03.jump`/`08.cse` transition before
the expensive final link. Changes elsewhere in the function must
still pass byte-by-byte validation, not just these four bytes.

Until stronger evidence exists, these two flags are accurate build
requirements for the current decompilation, **not evidence of which
compiler options Sony originally used**. Removing them now would
break the SHA despite preserving the apparent source-level intent.

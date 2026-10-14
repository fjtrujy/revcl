# VUC 2004.1 — Recover MLTempString C2 original-prefix source ownership

**Date:** 2026-10-15. **Repository:** `/path/to/revcl`
(branch `vuc`). Original evidence is the unmodified VUC 2004.1 Linux
ELF32 i386, verified through the existing C++03-era GCC 3.3.3 / GNU as
2.15 environment. All scores refer to the same original named function
and relocation-aware checker, never guessed whole-linked-ELF identity.

## Mandatory git/working-tree review

Started with `git status -uall`: 163 pre-existing unrelated untracked
paths, no tracked edits. Ran `git fetch origin && git rebase origin/main`
successfully; latest `origin/main` was already an ancestor. No extra
worktree, subagent, reset, cleanup, archive edits, or root VCL edits.

## Selected experimental MLTempString default constructor C2

Original symbol `_ZN12MLTempStringC2Ev` at original address
**`0x08071c20`**, original length **101 bytes**.

| Authentic comparison | Previous selection | Updated selection |
| --- | --- | --- |
| Compiler C++ owner | Isolated default C2 | Real formatted C2 predecessor then default |
| Source | `vuc/probes/ml_temp_default.cpp` | `vuc/probes/ml_temp_constructor_unit_oct12.cpp` |
| Historical flags | `-O2 -frename-registers` | `-O2` |
| Compiled original length | 101/101 | **101/101** |
| Identical original-offset instruction bytes | 95/101 | **95/101** (unchanged) |
| Contiguous byte-exact function entry prefix | 22/101 | **37/101** (+15 bytes) |
| Authenticated relocated sites | 9/9 | **9/9** |
| Status | Experimental | **Experimental; never entered exact manifest** |

The default constructor itself is unchanged C++03 semantics:
`text = MLTempStringData::Get()->FormatString("");`. Its `Get()`
inline initialization creates the original byte buffer and zeroes
the trailing counter. The real neighboring variadic constructor comes
first in the source unit and influences historical GCC register
assignment. The selected code's six remaining mismatched offsets are
**+37, +40, +55, +60, +66, +70**, versus the prior isolated
**+22, +28, +55, +60, +66, +70**. These correspond to register
choices and argument/initialization ordering; no original semantic
behavior is claimed recovered by a changed compiler switch alone.

The original `memset` and `FormatString` calls resolve to
`0x0804996c` and `0x08071a20`. The original empty string literal
is mapped at `0x08072286`. The inlined singleton guard and data
symbols match `0x0808bce8` and `0x0808c520`.
The independent matcher accounts for **nine function relocations**
including uses of singleton data. The sibling **exact 102/102 C1**
constructor in `vuc/src/MLTempStringDefault.cpp` is not changed.

The selected source was compiled *fresh*, outside of matrix result
objects, under **both** independent recovered GCC 3.3.3 prereleases,
January 25, 2004, and December 6, 2003, with as 2.15.
Each reproduces:
`length=101; exact_bytes=95; relocations=9;
different_offsets=[37,40,55,60,66,70]`.
This does **not** constitute a new fully byte-exact function.
The benefit is a larger byte-perfect prefix grounded in real
original-era source order, without losing any matching byte.

## Source experiments (negative controls included)

Four new reproducible drivers in `vuc/probes/` explore
ordinary C++03 source and period GCC controls:

| Source-level driver | Compiler recipes | Strict original-length result |
| --- | ---: | --- |
| `evaluate_value_tostring_dispatch_control_oct15.py` | 192 | Original 221-byte `Value::ToString`: still **159/221**; all 8 relocations and 82 original literal bytes verified |
| `evaluate_mltemp_c2_target_scheduler_oct15.py` | 204 | Three real constructor source contexts × GCC scheduler/architecture/branch options × both recovered snapshots: best **95/101**, 37-byte original prefix, 9 relocations |
| `evaluate_mltemp_c2_singleton_initializer_oct15.py` | 170 | C++03 `MLTempStringData` real inlined constructor and `memset` variants × GCC settings/snapshots: best **95/101**, 37-byte prefix |
| `evaluate_value_tostring_buffer_lifetime_oct15.py` | 105 | Real buffer alias/receiver/return/format lifetimes: `Value::ToString` remains **159/221**, with 8 relocs and 82 verified literal bytes |

**671 source/compiler recipes** were evaluated. All generated C++
and object files and individual JSON findings remain in the existing
Lima guest in correspondingly named directories under
`/tmp/vuc-binutils215/`, not committed. An initial scratch-only
relative header include mistake in the ValueToString buffer-lifetime
driver was corrected; the 105 intended source variants were then
compiled and rechecked. The MLTempString C2 historical driver
checks the correct **nine**, not eight, relocation sites.

Most alternative switch/if dispatch or buffer ownership forms either
produce the wrong original function length, fail literal/relocation
authentication, or retain the existing 159-byte score. The original
ToString function has a one-byte control-flow branch target difference
near +65 and another emission difference near the vector/single-float
formatting blocks, so its next useful hypothesis is authentic
translation-unit block layout rather than artificial padding.

The singleton argument-lifetime variations do not recreate the
original compiler's register selection. All discovered wrong-sized
alternatives remain negative evidence and are **not** promoted as
legitimate partials.

## Exact original verification

Reproduce the two independent accepted-constraint comparisons from
the selected C2 manifest:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 -O2 \
    -c "$R/probes/ml_temp_constructor_unit_oct12.cpp" \
    -o "/tmp/vuc-binutils215/c2-final-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/c2-final-${snap}.o" \
    --config "$R/config/ml_temp_default_C2_probe.json"
done
```

The independent exact suite remains **200/200** functions and
**14,930/14,930** original instruction bytes. Named data
**244/244 objects** (**71,536 bytes**), BSS **12/12** definitions
(**18,437 source allocation bytes**), and **162 Python regression
tests** remain green. The established 298-symbol inventory stays
**200 EXACT / 24 EXPERIMENTAL / 74 UNRESOLVED**.
The exact manifest and accepted sources remain untouched.

## Next step

Reconstruct the specific original source-unit/peephole register
allocation context for C2 to improve the six remaining bytes, or
continue independent targets with larger original-length partial
potential such as the original `Value::ToString` and hash Node
copy constructor. Never promote unchanged total matching bytes
as a new exact function, and never treat wrong-length overlap as a
legitimate strict partial.

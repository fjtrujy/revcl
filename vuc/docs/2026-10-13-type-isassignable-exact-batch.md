# VUC Type::IsAssignable: genuine 151-byte application-function match

Date: 2026-10-13. Branch: `vuc`.

## Byte-exact reconstruction

The original application method `Type::IsAssignable(Type const&, bool) const`
(`_ZNK4Type12IsAssignableERKS_b`) at original ELF address `0x0806a1c0`
now compiles to **151/151 original instruction bytes**, with no
relocations or unresolved literal operands. The accepted source is
`vuc/src/TypeAssignable.cpp`, compiled with the historical
GCC 3.3.3 (January 25, 2004) and GNU assembler 2.15 wrapper under
`-O2 -frename-registers -falign-jumps=4`. This is an actual named
application method, **not** a compiler-generated 31-byte startup wrapper.

The source contains the natural type equality and compatibility checks:

- Compare the two 32-bit flag words first.
- For unequal flag words, check that each masked variant (`& 0xffffc3ff`)
  has base value 4, and that the intersected flags equal the current
  type's flags; otherwise return false.
- For identical flag words, reject an incompatible pointer value when the
  original `0x100` flag is present, then check the actual boolean
  optional/allow state and return the result.

The original failed-probe implementation expresses the same operation
using three *early return* checks in the unequal-flags branch and
returns a comparison expression directly. With default 16-byte GCC
jump padding it emits **186 bytes**; using `-falign-jumps=1` drops to
around 150, but does not recover the precise original block ordering.
The successful version uses a **genuine nested conjunction of checks**:

```cpp
if (flags != other.flags) {
    if ((flags & 0xffffc3ffu) == 4 &&
        (other.flags & 0xffffc3ffu) == 4) {
        if ((other.flags & flags) == flags)
            return true;
    }
    return false;
}
```

That C++ control-flow structure gives GCC the original fallthrough and
false-branch shape. The historically supported four-byte jump alignment
resolves the remaining block-padding difference, producing the exact
151-byte original code. No assembly, padding declarations, invented
predecessor functions, address constants, static binary contents or
manual machine instruction edits are present.

## Reproducible positive and negative controls

`vuc/probes/evaluate_type_assignable_oct13.py` tests six genuine
C++ formulations (`early`, `early-a`, `guard`, `guard-noelse`, `nested`,
`original`) with five GCC options (`-O2` with/without renaming, or
explicit 1, 4 and 8-byte branch alignment). The **nested/4** candidate
matches all 151 bytes. Other formulations produce different code sizes,
even where the source-level semantics agree. This offers another
**GCC 3.3.3-specific** piece of evidence for the source control-flow
and local-lifetime techniques discussed in `main`'s
`config/matching-evidence/link-oparts-reconstruction.md` (which was
compiled with distinct GCC 3.2.2).

`vuc/probes/evaluate_accumulatorable_branches_oct13.py` additionally
tests eight normal source branch shapes for `IsAccumulatorable` under
five GCC options. Its best 77-byte candidate stays at **61/77**, and
no new exact match is claimed. The 40 generated option comparisons and
a previously saved authentic predecessor experiment are retained so
subsequent batches avoid blind repetition.

The existing `Translate(ASTNodeList<ASTStmt*>&)` original at
`0x0806ada0` was also reevaluated using its retained original-era
`vuc/probes/evaluate_translate_stmt_list.py`. The known C++ forms
produce predominantly **116-byte** or shorter variants against the
**119-byte** original, due to a different materialized return/result
local and changed basic-block placement. No previously accepted
symbol or unrelated VCL file has been changed for these negative
controls. The retained `MLTempString::Set` variants remain non-exact
as well; the smallest tested returns generate **118** bytes against
117 original. These partial results do not relax the strict acceptance
rules.

## Gates and whole-ELF limitations

This milestone raises accepted original-function coverage from 182 to
**183/298**, totaling **12,138/12,138 exact instruction bytes**.
Eight additional original functions remain experimental and 107
unresolved. The strict gate checks exactly the authenticated named
original address, length, emitted bytes and all required relocations
under the original 2004-era compiler. The independent initialized-data
and BSS acceptance gates must also remain green.

As with earlier independent source fragments, neither this 151-byte
exact method nor the preceding recovered wrappers establishes a
complete source-unit assembly, `.eh_frame` CIE/FDE grouping,
`STT_FILE` object ownership, native linker layout or full ELF
byte-for-byte identity. Those constraints remain open.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-type-183
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_type_assignable_oct13.py
```

# VUC 2004.1 — First strict original-length GetNextWord partial, and branch/iterator audit

Date: **2026-10-15**. Connected repository:
`/path/to/revcl`, branch `vuc`.
Original authority: unchanged archived VUC 2004.1 Linux ELF32 i386.
Compiler authority: GCC 3.3.3 (2004-01-25 and 2003-12-06 snapshots)
with historical GNU as 2.15 in the existing `revcl-oracle` Lima VM.

## Preservation / baseline

Started with a clean tracked worktree at `cc54729`, preserving **155
unrelated pre-existing untracked files**. Ran `git fetch origin &&
git rebase origin/main` before changes, with no new main commits.
Do not touch root VCL source, original archives, other sessions'
untracked files, accepted exact functions or the existing worktree
structure. Original VUC executable is immutable throughout.

The existing exact baseline is **201/201**, **15,007 exact original
instruction bytes**. New scanner work is diagnostic and selected only
in the *experimental* manifest. No whole-linked-binary byte identity
is claimed.

## Positive: previously unresolved GetNextWord now a partial

| Criterion | Verified result |
| --- | --- |
| Demangled method | `GetNextWord(char const*)` |
| Mangled name | `_Z11GetNextWordPKc` |
| Original entry | `0x080511e0` |
| Original length | **135 bytes** |
| Emitted C++03 candidate length | **135 bytes** |
| Identical original-offset machine-code bytes | **23/135** |
| Full function match | **NO** |
| Native emitted relocation sites | **5** |
| Authentic original named relocation targets | **3 unique globals** |
| Original `currentPosition` | `0x0808bc68` |
| Original `wordPosition` | `0x0808bc70` |
| Original `word` array | `0x08077440` |
| Original-length verified by | **both** GCC 3.3.3 snapshots |
| Flags | `-O2 -frename-registers -falign-jumps=16` |
| Selected status | **EXPERIMENTAL; never EXACT** |

The candidate source is
`vuc/probes/GetNextWordByteBoundsPartialOct15e.cpp`; matching
metadata is
`vuc/config/get_next_word_partial_oct15e_probe.json`.

Its genuine C++03 source expresses the source-level behavior supported
by original disassembly: skip **tab/newline/space** before the word,
update `currentPosition` and `wordPosition`, copy bytes into the
global buffer, and stop on NUL, tab, space, comma, period or left
bracket. Crucially, **newline inside a word is not a delimiter**.
No user-level runtime behavioral-equivalence test against the executable
has been established; the above is a static reconstruction.

The original scanner uses **EAX** for the running position, **ECX**
for input and **EBX** for output. This GCC source instead places
position in **ECX** and input in **EAX**, so the first mismatch is at
original offset `+1` and most bytes do not match. The semantic
structure and relocation graph, not a large byte-perfect prefix,
motivate tracking it as an experimental partial. Do **not** mistake
23 matching offsets for recovered original register allocation.

Earlier natural original-length scanner probes matched only
**8/135** at best. This new 23/135 score is a measurable,
strict-sized improvement over those unselected investigations,
not a new fully exact function.

Independently reproduce either historical compiler:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 \
    -O2 -frename-registers -falign-jumps=16 \
    -c "$R/probes/GetNextWordByteBoundsPartialOct15e.cpp" \
    -o "/tmp/vuc-binutils215/getnext-newpartial-${snap}.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/getnext-newpartial-${snap}.o" \
    --config "$R/config/get_next_word_partial_oct15e_probe.json"
done
```

Each independently outputs
`length=135`, `exact_bytes=23`, `relocations=5`,
`verified_literal_bytes=0`, `exact=false`.
Do **not** confuse 3 named globals with 5 actual emitted relocation
sites: `currentPosition` is referenced more than once.

The selected manifest gained **one** authentic partial; the exact
manifest remains unchanged. The entire 298-function ledger is now
**201 EXACT / 25 EXPERIMENTAL / 72 UNRESOLVED**.
Strict accepted source remains 15,007 machine-code bytes.

## Additional independent original ELF regression constraints

The three tests in
`vuc/tests/test_nonexact_original_branch_layout.py` read the
unmodified original ELF directly and verify:

- `Instruction.cpp` static arena cleanup `__tcf_1` at
  `0x08051980`, 46 bytes: the original epilogue starts with
  `59 5b 5d c3` (`pop %ecx; pop %ebx; pop %ebp; ret`).
  The current compiler produces an otherwise-identical
  `pop %ebx` at offset `+42`, so 45/46 is still
  experimental and not accepted.
- `Value::operator+(const Value&)` at `0x0806e9b0`, 219
  bytes: original x87 vector lanes are genuinely ordered
  0,1,2,3 at `+0xa7,+0xb0,+0xb9,+0xc2`, followed by
  the real assembler-emitted 3-byte alignment
  `8d 76 00` at `+0xcd`, before scalar block `+0xd0`.
  This does **not** authorize injecting assembly or source padding.
- STL normal-iterator scope uninitialized-copy original
  `0x0806a840`, **149** bytes: direct verification of its
  i386 hidden return-object storage and input argument register layout.

These checks are original-ELF evidence, not full binary-match
claims or assertions that diagnostic sources are exact.

## Genuine C++03 source and compiler matrices

Each diagnostic evaluator records candidate `.cpp`/`.o` and
`results.json` under its own VM
`/tmp/vuc-binutils215/<study>` folder. Sources are kept under
`vuc/probes/`. The results below explicitly separate
original-size/relocation-checked matches from wrong-length
overlap scores:

| Evaluator under `vuc/probes/` | Recipes | Authenticated result |
| --- | ---: | --- |
| `evaluate_tcf_real_neighbor_order_oct15e.py` | 360 | All 360 original length 46 with real 3 refs; best **45/46**, same single unused pop register |
| `evaluate_tcf_complete_original_neighbors_oct15e.py` | 204 | Uses accepted real Add/AddB/AddLineComment/GenerateImage neighbors, before/after cleanup; all original-size best **45/46** |
| `evaluate_value_plus_goto_lanes_oct15e.py` | 176 | Genuine negative-guard/goto source, natural original lane order; strict **172/219**, below selected **203/219** |
| `evaluate_value_and_goto_topology_oct15e.py` | 234 | Natural De Morgan + early-return CFG variations; strict best **192/325**, unchanged |
| `evaluate_value_division_goto_lanes_oct15e.py` | 1,008 | Actual x87 scalar/vector division and goto CFG, one original TypeConvert call; strict best **178/235**, unchanged |
| `evaluate_scope_iterator_large_alignment_oct15e.py` | 120 | Genuine libstdc++ implicit template, all six original EH/copy relocations; sizes 136,139,141,147,159,191, **none 149** |
| `evaluate_scope_iterator_eh_align_oct15e.py` | 96 | Real historical C++03 copy/destructor/clear exception specifications and compiler settings; **none 149** with all six authentic refs |
| `evaluate_get_next_word_byte_bounds_oct15e.py` | 500 | Correct scanner semantics with byte-sized branch and signed-style bounds; new **23/135**, 5 references, original length |
| `evaluate_get_next_word_compiler_passes_oct15e.py` | 86 | January and December GCC source/optimizer choices; 56 strict original-sized candidates, none better than **23/135** |

Total: **2,784** compiler/source recipes, including failures,
wrong-size artifacts and comparison controls. The original-sized
23-byte scanner is the only promoted candidate.
For example, 205 overlapping addition bytes in a **216-byte**
candidate are **not** 205/219 accepted partial bytes;
the true target requires all 219 bytes. A 237-byte
division candidate overlapping 212 original bytes is similarly
invalid. All candidate relocations must resolve to independently
authenticated original targets.

## Source/ABI findings and next direction

- `__tcf_1` register choice is not fixed by merely including or
  permuting the authentic predecessor methods; it is an i386
  postreload dead-stack-slot pop. Recover compiler RTL/source
  provenance, not arbitrary register pinning.
- `Value::operator+` requires an original-label-specific
  three-byte scalar-block alignment without padding the
  vector-case entry, and genuine ascending x87 vector lanes.
- Normal iterator scope-copy needs a **149-byte** EH/template shape.
  Merely adjusting `-falign-jumps`, `-falign-labels`,
  `-falign-loops` to 32 does not recover that shape.
- The scanner's next key problem is input/position register
  lifetime. Source-level pointer/position ownership changes may
  improve actual original byte offsets; avoid explicit x86
  register binding or fake machine-code byte sequences.

## Acceptance procedure

```sh
python3 -m unittest discover -s vuc/tests -q
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 "$R/scripts/match_candidates.py" \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/getnext-oct15e-final-exact
limactl shell revcl-oracle -- python3 "$R/scripts/match_data_candidates.py" \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/getnext-oct15e-final-data
limactl shell revcl-oracle -- python3 "$R/scripts/match_bss_candidates.py" \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/getnext-oct15e-final-bss
```

Expected exact acceptance remains **201/201 functions,
15,007/15,007 original code bytes**, unchanged **244/244
initialized data objects** (71,536 bytes), unchanged **12/12
BSS** source declarations (18,437 allocated bytes); host
tests now include four added checks: three original-ELF
branch/layout tests and the strict scanner manifest assertion.

No claim about byte-exact linked whole executable.

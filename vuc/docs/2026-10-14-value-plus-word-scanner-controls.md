# VUC: arithmetic alignment and word-scanner register experiments

Date: 2026-10-14. Branch: `vuc`. Starting commit:
`ff1f784`; original VUC 2004.1 Linux ELF remains untouched
inside the pinned upstream ZIP.

## Outcome

**No new accepted byte-exact function or improved original-length
partial emerged from this batch.** The tests did produce a concrete
control-flow/layout explanation for the existing Value addition
partial and a closer *semantic* GetNextWord byte-operand hypothesis.
These are not grounds for exact promotion.

No artificial source padding, handcrafted machine instructions,
binary patching, fabricated relocations, inline register constraints,
or dummy predecessor functions were used.

## Value::operator+(): original 219-byte alignment asymmetry

The original function at `0x0806e9b0` and the natural C++ source in
`vuc/src/ValueArithmeticUnit.cpp` generate the same 32-byte
Value-object initialization, original `TypeConvert` call,
integer and x87 scalar/vector addition, and compact return block.
The mismatch is mainly compiler-generated branch alignment:

| Location relative to function | Original ELF | Natural GCC result |
| --- | --- | --- |
| Vector-case comparison | `+0xa2` | `+0xa2` when jumps unaligned; `+0xa4` at jump alignment 4 |
| Vector arithmetic start | `+0xa7` | `+0xa7` or `+0xa9`, respectively |
| Scalar-case entry | `+0xd0` | `+0xcd` when jumps unaligned; `+0xd0` at jump alignment 4 |
| Method size | **219 bytes** | 216 with `-falign-jumps=1`; 219 with `=4` |

The original has the **unpadded vector case** but the **padded
scalar case**. The natural `=1` candidate omits padding before
both cases; `=4` inserts two extra padding bytes before the
vector comparison and one before the scalar comparison, changing
branch targets. This is an *observed compiler-output difference*,
not justification for inserting manual NOPs.

`vuc/probes/evaluate_value_plus_alignment_oct14.py` tests
**80** period-GCC jump/label/loop alignment combinations with
the real `+`, `-`, `*` method order, keeping the verified
`TypeConvert` relocation. The best exact-length candidate
remains **172/219 matching original instruction bytes**.
Some other 219-byte combinations score 163/219 or 140/219.
No proposed change is added to the accepted exact or selected
experimental manifest; the existing partial record remains
authoritative.

This is a useful boundary for the next attempt: the original
compiler must treat the scalar-case label differently from the
vector-case target. Reconstructing authentic branch probability
or actual source-unit compiler state is preferable to global
alignment-force sweeps.

## GetNextWord(): natural byte-operand dispatch, still not original

Original function at `0x080511e0` is **135 bytes**.
It loads `currentPosition`, skips tab/newline/space, updates
`wordPosition`, and copies bytes until one of NUL, tab, space,
comma, dot, or left bracket. Unlike the leading-whitespace loop,
its in-word loop does **not** terminate on newline.

The original assembly compares the byte in `DL`, keeps position
in `EAX`, input in `ECX`, and output in the callee-saved `EBX`.
The old `switch` source tends to promote the character to
a 32-bit comparison and assign different pointer/counter registers.

`vuc/probes/evaluate_get_next_word_switch_registers_oct14.py`
tests **240** natural C++03 leading-whitespace, delimiter tree
and character-copy ordering combinations under historical
alignment and register-renaming flags. A nested 8-bit comparison
version recovers the original *byte-sized comparison form* and
uses `EBX` for output, but the generated method is **127 bytes**,
shorter than the original; no original-length candidate improves
the existing independent source evidence. The new original-sized
candidates in the sweep match at most **8/135** under the
strict original-address comparison; they are not promoted.

`vuc/probes/evaluate_get_next_word_local_registers_oct14.py`
then tests **252** real C++ local-variable representations,
C++03 `register` hints, parameter declarations, increment orders
and return-lifetime variants starting from the 127-byte nested
source. **None emits the required original 135-byte size.**
Register hints are normal historical C++ language features,
not inline assembler register forcing; they were tested only
as negative hypotheses, not accepted.

Further useful work would recover the original expression/order
that retains the position in `EAX` and input in `ECX`,
rather than trying to inflate the 127-byte result artificially.

## Strict acceptance

The expected authoritative ledger is unchanged:
**195 EXACT / 15 EXPERIMENTAL / 88 UNRESOLVED**
(out of 298 functions), with **13,992** previously accepted
original instruction bytes. The independent data and BSS
acceptance gates cover 244 initialized objects / 71,536 original
bytes and 12 BSS declarations.

Run the historical acceptance scripts in the existing
`revcl-oracle` VM:

```sh
R=/path/to/revcl/vuc
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_value_plus_alignment_oct14.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_get_next_word_switch_registers_oct14.py"
limactl shell revcl-oracle -- python3 \
  "$R/probes/evaluate_get_next_word_local_registers_oct14.py"
python3 -m unittest discover -s vuc/tests -q
```

Full original linked-ELF byte identity is still **not established**.

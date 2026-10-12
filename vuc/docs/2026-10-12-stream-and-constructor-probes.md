# VUC stream, scanner, and original constructor-unit probes

Date: 2026-10-12. Branch: `vuc`.

These results are negative **source-reconstruction experiments**, not accepted
byte-exact functions. All comparisons use the authenticated ZIP-backed
original VUC ELF and the historical January 2004 GCC 3.3.3 / GNU as 2.15
compiler wrapper in the `revcl-oracle` VM.

## Smallest outstanding ostream function

`AsmOutput_WritePrologue(std::ofstream&)` at `0x08049b40` is 30 original
instruction bytes. Its original function loads a string-literal address
into EAX and the stream argument into EDX, then passes those values to
`std::operator<<`. The reconstructed C++ emits an equivalent call, but the
compiler schedules the stream-argument load and parameter stores differently.

`vuc/probes/evaluate_asm_prologue_call_shapes_oct12.py` tests **44**
ordinary C++ variants: eleven member/free operator-insertion expressions,
stream references and pointer locals, with four historical flag choices.
The best remains **22/30** matching bytes. All tested functions have the
correct 30-byte length; no byte-exact version was found. The probe config
maps its candidate `.rodata` section relocation to the authenticated
original literal location as a diagnostic input; it does not fabricate a
new address or count the original 92-byte text as a function match.

## Original inline-assembly word scanner

`GetNextWord(char const*)` at `0x080511e0` is 135 bytes. The original
scanner has distinct whitespace skipping and token-terminator dispatch
paths. Its disassembly contains repeated comparisons for comma (0x2c)
and tab (0x09), which suggest original switch/branch formulation rather
than an optimized set-membership test.

The preserved `evaluate_get_next_word_oct12.py` checks **112** conventional
C++ whitespace/terminator and flag combinations. None reproduced the
original 135-byte function length. It remains in `vuc/probes` and is
**not** considered reconstructed.

## Historical string constructor ordering

The original `MLTempString` formatted and default constructors appear in
symbol order as distinct C1/C2 functions. The original default C1 has
already passed **102/102**; default C2 is **101** bytes and remains
experimental at **95/101** with the isolated original-era source fragment.

`vuc/probes/ml_temp_constructor_unit_oct12.cpp` joins the two constructor
bodies in that apparent source order with one shared inline
`MLTempStringData::Get()` singleton definition. Recompiling the combined
translation unit alters register allocation but worsens the accepted
constructor: default C2 now emits **102** instead of 101 bytes, and default
C1 is **100/102**, not exact. The combined source must therefore never
replace `vuc/src/MLTempStringDefault.cpp`. Recovering the original source
neighbors/translation unit is still required.

The separate `vuc/probes/instruction_add_format_oct12.cpp` explores a
named `MLGameString` temporary for `InstructionList::Add(char const*,...)`
at `0x08051570`. It emits **186** bytes rather than the original **146**:
the compiler inserts additional exception-construction state. The
original new/temporary construction ordering remains unresolved; this
source cannot enter the exact manifest.

## Value addition and general pending sweep

`evaluate_value_plus.py` checks **42** ordinary `Value::operator+()`
control-flow/alignment hypotheses at `0x0806e9b0` (219 original bytes).
Its best variant reproduces **172/219** instruction bytes with exact
function length. The already accepted subtraction operator is unaffected;
addition must not be promoted on the strength of a partial match.

Further checks of the 77-byte `IsAccumulatorable()` predicate show why its
previous **61/77** candidate is deceptive. With
`-falign-jumps=1`, the original's case-12 branch target begins four bytes
later than the candidate, although subsequent case-13/case-11 blocks and
the final return path agree. Using eight-byte jump and 16-byte loop
alignment produces an 83-byte function, and 16-byte jump/loop alignment
produces 91 bytes: both also align later branch targets absent from the
original. Neither is accepted. The original control-flow structure and
source-level branch probabilities, not cosmetic code padding, must explain
the selective alignment.

`recheck_pending_probes.py` was rerun with the historical compiler after
these investigations. It checked **44** pending configs and found **zero**
additional exact, unaccepted symbols. Partial results remain diagnostic,
and configs describing data objects are not function candidates.

## Verification and priorities

The accepted manifest remains **166 EXACT**, **9 EXPERIMENTAL** and
**123 UNRESOLVED** among 298 original named function records. The
independent strict gate continues to accept **166/166** functions,
**11,314/11,314** instruction bytes and 417 supported relocations.
The separate named-data and BSS source gates also pass. None of these
checks establishes a matching linked executable. Further investigations
should prioritize authentic historical translation-unit context and source
semantics, rather than repeating equivalent compiler-flag sweeps.

Reproduce the diagnostics in `revcl-oracle`:

```sh
r=/path/to/revcl/vuc
cc=/tmp/vuc-binutils215/g++-jan25-as215
python3 "$r/probes/evaluate_asm_prologue_call_shapes_oct12.py"
python3 "$r/probes/evaluate_get_next_word_oct12.py"
python3 "$r/probes/evaluate_value_plus.py"
python3 "$r/scripts/recheck_pending_probes.py" --compiler "$cc" \
  --out /tmp/vuc-binutils215/pending-oct12-resume2
```

All scratch compilation outputs remain in the VM under `/tmp`; the
canonical archive and user-supplied extracted binary are unchanged.

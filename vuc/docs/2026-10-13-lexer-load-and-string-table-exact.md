# Exact VUC lexer buffer loading and string-table singleton

Date: 2026-10-13, branch `vuc`.

## `yyFlexLexer::yy_load_buffer_state()` — 39/39 bytes

Original `0x08061780`, 39 instruction bytes, zero relocations.
The accepted body is a normal C++ definition in the already recovered
`vuc/src/FlexLexer.cpp` translation-unit fragment. Its five assignments
are:

```cpp
yy_n_chars_ = current_->yy_n_chars;
yy_c_buf_p_ = current_->yy_buf_pos;
yytext_ptr_ = yy_c_buf_p_;
input_ = current_->file;
yy_hold_char_ = *yy_c_buf_p_;
```

Original GCC 3.3.3 with `-O2 -frename-registers` chooses EBX for the
current buffer, EDX for its current character pointer and ECX for
`this`, reproducing the original function's EBX save/restore and every
instruction byte. The original 37-byte compiler output from the
previously reasonable source order did not preserve that lifetime.

The reproducible experiment `vuc/probes/evaluate_yyload_source_order_oct13.py`
iterates semantically meaningful permutations of five independent
lexer-field updates, skipping sequences that read the current cursor
before writing it. It finds the exact source ordering named
`simple-02314` (original fields numbered 0=count, 1=input,
2=cursor, 3=text, 4=hold). The accepted source is integrated with
the genuine already accepted lexer methods: the same compiler object
still matches `LexerOutput()` **18/18**, `LexerError()` **54/54**,
`yy_switch_to_buffer()` **75/75**, `yy_init_buffer()` **53/53**,
`yy_delete_buffer()` **87/87**, and both base destructor clones.
The original `yy_flush_buffer()` remains separately accepted with
its previous `-falign-jumps=1` flag. The historical nonmatching lexer
probe is preserved at `vuc/config/yy_load_full_unit_oct14_probe.json`
but is no longer selected for the experimental coverage ledger.

## `MLGameStringTable::Get()` — 106/106 bytes and nine relocations

Original `0x08071810`, 106 bytes. A genuine original-era lazy-initialized
static table with 513 hash buckets, two 32-bit state counters, a guard
byte and an atexit cleanup was already reconstructed to **92/106**
bytes in `vuc/probes/ml_string_table_get.cpp`. The compiled source
had the original 106-byte length and nine independently mappable
ELF32 relocations, but its counter writes and register choices differed.

The source-unit experiment `vuc/probes/evaluate_ml_string_table_get_unit_oct13.py`
combines the real, previously byte-exact `MLGameStringData`,
`MLGameString`, and `MLTempString` methods before the original table's
`Get()` body. These are genuine original methods, not invented prefix
fillers. The resulting GCC 3.3.3 code had **104/106** matching bytes
under `-O2 -frename-registers -fuse-cxa-atexit`.

The last two mismatches were **not application instructions**. They
were bytes 90–91 of the relocation-backed pointer passed as the
`__cxa_atexit` destructor callback. The old probe's synthetic
`.text` section-base mapping was only valid for an isolated object
where `__tcf_0` was at section offset `0x70`. The genuine source unit
places that callback at offset `0x350`, changing the meaning of that
section-base mapping. Forcing a new arbitrary text-section base would
have been misleading.

Compiling the full source fragment with `-ffunction-sections` gives the
callback its own `.text.__tcf_0` section. The original callback is a
*known named original function* at `0x08071e00`, and its relocation can
therefore be checked directly against that authenticated address.
The complete candidate is now **106/106 bytes** with **all nine
relocations** exact. This is recorded in
`vuc/config/ml_string_table_get_probe.json` and
`vuc/src/MLGameStringTableUnit.cpp`.

The same emitted source-unit object independently verifies the
already accepted cleanup `__tcf_0` **22/22** with two relocations,
`MLGameString` default constructor **14/14**, and its copy constructor
**15/15**; the separate original acceptance cases remain unchanged.
No original ELF, instruction bytes, invented relocation targets, or
manual linker sections were patched. This establishes per-function
matching, **not** byte-identity of a whole historical source unit.

## Other tested applications, not newly accepted

Eight plausible real branch forms of `IsAccumulatorable()` under five
original-era GCC flag combinations still produce at best **61/77**.
The 119-byte `Translate(ASTNodeList<ASTStmt*>&)` remains an unresolved
spilled-result/local-live-range reconstruction; source variants mostly
compile to 116 bytes. A wider variadic `MLTempString::Set()` probe
series still produces 118 or 110 bytes against 117 original bytes.
The additional original C++ constructor-context control
`vuc/probes/evaluate_mltemp_default_context_oct13.py` tests actual
`MLTempStringData::FormatString`, `MLTempString::Set`, and `operator+=`
definitions in different source orders for the 101-byte C2 default
constructor. Variants without added literal sections remain at best
**95/101**. Variants adding the genuine `"%s%s"` string are deliberately
rejected by the relocation-literal guard until every new source-unit
literal has an authenticated original mapping. Neither source-context
code nor optimization flags are sufficient to accept that C2 clone.
These unsuccessful paths are preserved in the versioned `vuc/probes`
experiments; no nonexact function is added to strict acceptance.

## Acceptance and limitations

Together with the already accepted 151-byte `Type::IsAssignable`, the
current batch adds **three real application functions and 296 exact
instruction bytes**. The original 298-function coverage ledger is now
**185 EXACT**, **6 EXPERIMENTAL**, **107 UNRESOLVED**. The strict
function gate checks **12,283/12,283 instruction bytes** with
relocations; the initialized-data and BSS gates are independent.

The original linked ELF still cannot be rebuilt byte-identically.
The accepted C++ source-unit fragments may overlap, and full linker,
exception-section, and original compilation-unit provenance remain
unverified. Those must not be inferred from individual function scores.

Reproduction:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-lexer-string-table-185
```

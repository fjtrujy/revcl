# VUC 2004.1 — yy_create_buffer fully byte-exact with genuine yyrestart predecessor

Date: 2026-10-15. Target: original archived VUC 2004.1 Linux ELF32 i386. Historical compilers: recovered GCC 3.3.3 January 25, 2004 and December 6, 2003 snapshots with GNU assembler 2.15. Source is authentic C++03 without inline assembly, manufactured padding, hard-register controls or fabricated references.

## Git and preservation

At entry, branch `vuc` was at `2e82ae4` and the tracked tree was clean. There were exactly 155 unrelated untracked paths under `git status --porcelain=v1 -uall`. Fetched `origin` and rebased 144 local VUC commits onto updated `origin/main` `63b12da`, without conflicts. The prior GetNextWord 50-byte partial became rebased commit `054eeee`. Existing unrelated untracked paths were not modified, staged or deleted.

## New exact function

Original `yyFlexLexer::yy_create_buffer(std::istream*, int)` (`_ZN11yyFlexLexer16yy_create_bufferEPSii`) at `0x080617b0` has 139 bytes. The prior original-length candidate matched 131/139 with five authenticated relocations and 44 literal bytes. A new historical compiler probe discovered that placing the *genuine* 93-byte `yyFlexLexer::yyrestart()` definition before `yy_create_buffer()` naturally changes the compiler's dead register choices in both failure branches. Both functions are **fully byte-exact in the same compilation unit**.

- Accepted source: `vuc/src/FlexLexerCreateBufferUnit.cpp`.
- Config: `vuc/config/yy_create_buffer_probe.json`, historical `-O2 -frename-registers`.
- Result under **both** recovered GCC snapshots: 139/139 original instruction bytes, original function length 139, five authenticated ELF relocation sites, 44 verified original literal bytes, `exact=true`.
- Co-compiled original `yyrestart`: 93/93 bytes, two original relocations, `exact=true`; it was already accepted in a separate source fragment.
- The source uses ordinary 2004-era flex-generated class and buffer-state declarations, `malloc`, virtual `LexerError`, `yy_init_buffer`, and conventional field assignment.
- The accepted exact manifest grows from 201 to **202 functions** and from 15,007 to **15,146 exact function instruction bytes**. Inventory: 202 EXACT, 24 EXPERIMENTAL, 72 UNRESOLVED, 298 total. The matched named data/BSS inventories remain separate.

The fully reproducible source-unit search driver is `vuc/probes/evaluate_yycreate_true_prefix_oct15g.py`. It evaluates real lexer predecessor selection and historical flags. The successful first positive case was `restart-o2rename`, after a standalone negative control of 131/139. The driver writes its source, object and score ledger into the Lima guest `/tmp/vuc-binutils215/yycreate-true-prefix-oct15g/` and stops on fully exact comparison.

Reproduction:

```sh
R=/path/to/revcl/vuc
for snap in jan25 dec06; do
  limactl shell revcl-oracle -- \
    /tmp/vuc-binutils215/g++-${snap}-as215 -O2 -frename-registers \
    -c "$R/src/FlexLexerCreateBufferUnit.cpp" \
    -o "/tmp/vuc-binutils215/flex-create-real-${snap}-oct15g.o"
  limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
    --object "/tmp/vuc-binutils215/flex-create-real-${snap}-oct15g.o" \
    --config "$R/config/yy_create_buffer_probe.json"
done
```

## Secondary control: Value addition genuine predecessors

`vuc/probes/evaluate_value_add_real_predecessors_oct15g.py` tested 70 authentic C++03 source/flag combinations involving true `Value::Set(ASTExpr const*)` and `Value::Set(ASTLValExpr const*)` predecessor bodies plus vector lane orders. Forty-five retained original length and the authenticated relocation. The best remained the existing **203/219** Value addition partial; none reached exact. Generated diagnostic objects and JSON are under guest `/tmp/vuc-binutils215/value-plus-real-prefix-oct15g/`. No existing accepted Value source or selected candidate changed.

## Acceptance scope

This is a fully byte-exact **function**, not a claim that the whole 314,338-byte ELF is identical. The exact function suite, host unit tests, initialized data and BSS must pass after promotion. The old experimental `yy_create_buffer.cpp` source remains preserved as negative baseline evidence under `vuc/probes/`.

# VUC 2004.1 — actual hash-node aliasing and real script_error unit

Date: 2026-10-15. Branch: `vuc`, based on latest `origin/main` at `88ecfc3`. Original Linux x86 ELF and Jan25/Dec06 GCC 3.3.3 with as 2.15 are authoritative. No inline assembly, register pinning, synthetic NOPs, fabricated symbols, fake relocations or new worktrees.

## SymbolTable::Enter: old-head/new-head stores are alias-sensitive

The original 185-byte Enter code calculates a hash bucket, writes `node->next = heads[bucket]`, then writes `heads[bucket] = node`. The provisional `SymbolNode` class is distinct from the actual `MLHashTable::Node` class, so GCC 3.3 reorders these independent memory writes. The original tail at `+0x99` is `mov edx,[ebx+ecx*4]; mov [esi+0xc],edx; mov [ebx+ecx*4],esi`, whereas the provisional version stores the bucket head before `node->next`.

The new `vuc/probes/evaluate_symbol_enter_real_node_alias_oct15k.py` tests actual hash-node-type aliases for the old/new head and probes normal C++03 constructor lifetime. In an actual-node-type store variant, GCC reproduces the **original store ordering**, but the provisional constructor `SymbolNode(...): ..., next(0)` emits an additional seven-byte `mov [esi+0xc],0` before the hash lookup. Removing the unnecessary `next(0)` initialization (which is immediately overwritten in all published nodes) eliminates those seven extra bytes while retaining the original store order. The generated original-candidate `Enter` then returns to **180 bytes** with `-falign-jumps=1` (original 185), or **186 bytes** with `-falign-jumps=8 -falign-loops=1 -falign-labels=1`. The original five-byte allocation-branch alignment has still not been produced *without also changing other code*, so this is **not exact**.

The driver now checks eight authentic combinations of null-initialized/deferred-next construction and normal/alias/volatile-link access, each under two historically motivated fixed compiler layouts: **16 comparisons**, all preserving authenticated address and exact-function boundaries. The entire naturally instantiated `std::vector<SymbolScope>::_M_insert_aux` remains **733/746** with 15 relocations when its 746-byte size is preserved. Two further natural C++03 nested-link type spellings produced the same results. Full generated C++03 and compiled ELF32 objects and scores are retained in `/tmp/vuc-binutils215/enter-node-alias-oct15k/`.

**Interpretation:** a genuinely typed hash-table node and deferred link initialization are more plausible original source than the provisional cast-heavy `SymbolNode`. Nevertheless, the unresolved Enter branch alignment and link epilogue block still prevent promoting a 185-byte candidate to the exact manifest. No accepted application code was changed.

### Actual nested `MLHashTable::Node` constructor control

The further reproducible driver `vuc/probes/evaluate_symboltable_real_nested_node_oct15k.py` uses a scratch-only copy of the actual `vuc/src/MLHashTable.h`, adding a period-plausible three-argument `Node` constructor (no redundant `next=0` store). It then compiles `SymbolTable::Enter` allocating **`SymbolScope::Node` directly**, with the genuine neighboring `Find`, `BeginScope`, and `EndScope` methods. No accepted header is modified, and both recovered GCC snapshots are checked.

Under `-O2 -frename-registers -falign-jumps=1`, each snapshot reproduces **115/115 Find, 140/140 BeginScope, 27/27 EndScope, and 733/746 Scope insertion** with the original 15 relocations; the nested-node `Enter` remains 180 bytes against 185 original. The jump-8 / loop-1 control changes the candidate Enter to 186 bytes and the vector helper to 745, so these are not accepted. This independently shows that a more faithful nested hash-node type alone does not resolve the remaining code placement and register lifetime mismatch. Generated sources, objects and JSON ledger are retained under `/tmp/vuc-binutils215/symboltable-nested-node-oct15k/`.

## Scope uninitialized-copy helper: original 149 bytes

Original `std::__uninitialized_copy_aux<normal_iterator<SymbolScope*>,normal_iterator<SymbolScope*>>` at `0x0806a840` has **149 instruction bytes**, with six genuine call relocations to the scope copy constructor, `clear`, `__cxa_begin_catch`, `__cxa_rethrow`, `__cxa_end_catch` and `_Unwind_Resume`.

The same authentic `SymbolTableUnit.cpp` currently instantiates a **145-byte** helper with the same relocations and semantics. Original disassembly shows four bytes of compiler branch-target alignment at the transition into the exception cleanup path that are not present in the candidate. A targeted, fixed-source compiler study of the historical jump/label/loop alignment interactions generated 137-, 139-, 143-, 145-, 147-, 151-, 159- and 191-byte candidates, but **no 149-byte function**. The full gate continues to reject all candidates by original-size mismatch. The generated objects and bounded result ledger are retained under `/tmp/vuc-binutils215/scope-uninit-align-oct15k/`.

This is negative evidence, not a justification for source-level artificial padding. The original 149-byte helper remains unresolved.

## Authentic script_error source-unit partial: more literal provenance

The original `script_error(const char*)` at `0x0806fa80` is **161 bytes**, with 15 relocation sites. Its isolated experimental source `vuc/probes/script_error.cpp` had **155/161** original-offset instruction bytes and five authenticated literal bytes, with mismatches `+42,+48,+99,+105,+131,+137`.

The genuine `vuc/src/VucCompilerUnit.cpp` contains `CurrFileInitialization.cpp`, `ScriptLexer.cpp`, the original diagnostic source hypothesis and `Main.cpp`. Its assembled `.rodata` section is six bytes long: an empty NUL literal at offset zero, `"("\0` at +1, and `"):\0"` at +3. The extra empty literal is **not guessed**: `vuc/config/data_currFileName.json` authenticates the original `currFileName` string pointer to the exact one-byte NUL literal at **0x08072286** in the source ELF. The two original diagnostic literals are independently authenticated at **0x08075c5d** and **0x080766dc**.

The selected, strictly **EXPERIMENTAL** manifest `vuc/config/script_error_probe.json` now uses `vuc/probes/ScriptErrorRealUnitOct15k.cpp`, a thin include of the actual compiler unit. It maps all three original literals by their original offset and provenance. Both Jan25 and Dec06 GCC snapshots with `-O2 -frename-registers -falign-jumps=1` reproduce exactly **155/161** original-size bytes, **15/15** original relocation sites and **6/6** authenticated literal bytes. This is a strict improvement in original source-unit and literal verification, but **not another byte-exact function**. Without the supported `-falign-jumps=1` condition, the function emits 174 bytes, correctly rejected.

New reproducible driver: `vuc/probes/evaluate_script_error_real_tu_oct15k.py`. It derives the empty-literal original address from the *existing authenticated named data manifest*, validates all literal ranges and compiles both recovered GCC snapshots. It leaves generated C++ objects and a full JSON ledger in `/tmp/vuc-binutils215/script-real-tunit-oct15k/`. A host test prevents silent regression to an isolated literal context.

## Regression and next target

This batch has not added a new byte-exact function. Keep the inventory **204 EXACT, 25 EXPERIMENTAL, 69 UNRESOLVED**; the strict `Scope` insertion remains 733/746. The best next hypothesis is to recover real historical `MLHashTable::Node` constructor and original `SymbolTable::Enter` method boundaries instead of adding machine-code alignment. The complete linked VUC executable is still not byte-exact.

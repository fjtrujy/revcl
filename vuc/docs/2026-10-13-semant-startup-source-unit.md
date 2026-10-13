# Semant.cpp: byte-exact global startup wrapper from real semantic checks

Date: 2026-10-13. Branch: `vuc`.

The original compiler-generated `_GLOBAL__I__Z20CheckActualParamListRK11ASTNodeListIP7ASTExprERKSt6vectorI4TypeSaIS6_EERK12MLGameString` at `0x0806a0c0` is now independently **31/31 byte-exact** with its single ELF32 relocation resolved to `__static_initialization_and_destruction_0(int,int)` at `0x080619d0`. Its compilation unit `vuc/src/SemantUnit.cpp` uses ordinary C++03 source compiled with recovered GCC 3.3.3 (January 25, 2004) and GNU assembler 2.15 under `-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`.

The candidate contains a source-level, deliberately provisional real argument-list validator keyed to the original first Semant.cpp function, `CheckActualParamList`, followed by two already accepted genuine methods. The same compiler object retains `Check(ASTFunCall*)` **118/118** with three relocations and `Semant_Check(ASTModule*)` **40/40** with two relocations. The first method handles count and type comparisons and formats a diagnostic on disagreement. Its emitted body is **210 bytes versus the original 2,510**; it is emphatically **not** a decompilation acceptance, and the true original error policy and semantic type checks are unresolved.

Initial source material and the original preserved check implementations are available in `vuc/probes/semant_real_unit_first_oct13.cpp`, `vuc/src/CheckASTFunCall.cpp`, and `vuc/src/SemantCheck.cpp`. The experiment also leaves `vuc/probes/semant_real_unit_oct13.cpp` as a non-accepted alternative source order. Exact manifest promotion is restricted to the original 31-byte compiler-generated function; no fabricated code, instruction-level patches, padding, invented relocations, or archived binary changes were used.

This increases original-function coverage to **176 EXACT**, **9 EXPERIMENTAL**, **113 UNRESOLVED** (298 records). All 176 accepted records together cover **11,801/11,801 instruction bytes**. It does not establish whole-ELF byte identity or an authenticated original Debian toolchain patchset.

Reproduction:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-semant-176
```

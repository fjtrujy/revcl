# VUC rebase and five byte-exact startup function records

Date: 2026-10-13. Branch: `vuc`.

## Safe update from VCL `main`

Fetched `origin/main` at `1825ea7` (`Complete link_oParts.cpp matching at 0x08054f74`) and rebased all **86** VUC-only commits cleanly. Before the rebase the tracked checkout was clean; **136 pre-existing untracked paths**, including the extracted original binary and incomplete scratch experiments, remained untouched. No worktrees or subagents were created. The original VUC archive and its SHA256 identity were not changed.

The new main evidence in `config/matching-evidence/link-oparts-reconstruction.md` and its compiler/measurement files is unusually instructive:

- A **genuine, already-needed named C++ receiver alias**, together with a contiguous real source-unit prefix, changed the GCC 3.2.2 register interference and peephole2 scratch selection enough for the 665-byte VCL function to become exact. Neither the alias alone nor the source partition alone matched. This supports constrained experiments with *real* C++ local receiver lifetimes, original function order and RTL pass dumps; it does **not** establish that GCC 3.3.3 behaves identically.
- The VCL reconstruction explicitly notes that the historical translation-unit boundary is **not proven**. It preserved other exact functions and protected data, but its **whole-linked-file score fell by 347 bytes**; CIE/FDE and `.eh_frame` grouping, extra `.comment` input tags, string-section alignment and relocations changed despite a 665/665 relocation-resolved function. We must track VUC's object provenance, literals, `.eh_frame`/LSDA, startup files and native relocation destinations before claiming any ELF progress.
- Diagnostic-only `-da` dumps were checked byte-identical to VCL's production object. For VUC, use the separately verified GCC 3.3.3 `-da` pass numbering (`.27.peephole2`, `.28.rnreg`) and historical assembler wrapper. Do not import GCC 3.2.2 flags, synthetic prefixes, or VCL authorizations as VUC facts.

## Newly accepted original function records

The following functions are **31/31 original instruction bytes**, all with the named original ELF32 relocation to their correct local `__static_initialization_and_destruction_0(int,int)`:

| Original function (startup keyed to) | Address | Original helper target | New ordinary C++ source |
| --- | --- | --- | --- |
| `BindInlineAsm(ASTAsmStmt*, SymbolTable&)` | `0x08051270` | `0x08050ce0` | `vuc/src/InlineAsmUnit.cpp` |
| `BroadcastMove(IR::Register const&, IR::Register const&, InstructionList&)` | `0x080561a0` | `0x08051ae0` | `vuc/src/IR2AsmBroadcastUnit.cpp` |
| `AssembleBinOpFloat4(IR::Register const&, IR::Expr_Data*, InstructionList&)` | `0x0805a4d0` | `0x080561c0` | `vuc/src/IR2AsmBinExprUnit.cpp` |
| `script_debug` global from `script_parser.cpp` | `0x08060790` | `0x0805cce0` | `vuc/src/ScriptParserUnit.cpp` |
| `yyFlexLexer::yylex()` global from `script_scanner.cpp` | `0x08061980` | `0x080607c0` | `vuc/src/ScriptScannerUnit.cpp` |

These checks use authentic GCC 3.3.3 (January 25, 2004) with GNU assembler 2.15. For the first four, flags are `-O2 -frename-registers -fuse-cxa-atexit -ffunction-sections`. **The scanner requires `-O2` without `-frename-registers`**: the original initializer loads `0xffff` into EAX, whereas `-frename-registers` in this tested source selects EDX, leaving 29/31 matching bytes. The original application code is large and its complete per-TU flags remain unauthenticated, so these are *per-function compiler-hypothesis flags* and not historical Makefile assertions.

The underlying application source is **partial/provisional**. The original named functions are 450-byte `BindInlineAsm`, 416-byte `BroadcastMove`, 9258-byte `AssembleBinOpFloat4`, 14922-byte `script_parse`, and 1698-byte `yyFlexLexer::yylex`. Their present C++ fragments model meaningful original operations and APIs (inline-assembly token parsing and binding, register-to-register instruction construction, IR floating-point operator dispatch, token-state parsing, and lexer buffer scanning), but none reproduces the complete original application function or authenticates its full control flow. **Only the five generated 31-byte wrappers are exact or promoted.** The large original methods are not counted as exact; their provisional statements are source-level hypotheses, not demonstrated disassembly equivalence.

Some source fragments duplicate independently reconstructed methods to reproduce historical compiler context. They are *not* a linkable reconstruction yet: completing legitimate source-unit ownership, matching `STT_FILE`/headers and resolving overlapping C++ definitions are still necessary. There is no fabricated machine-code sequence, assembly patch, artificial padding object, original binary rewriting, or altered matching comparator.

## Receiver-lifetime negative control

To apply the new VCL lesson without blindly transplanting its compiler mechanism, `vuc/probes/evaluate_printer_receiver_lifetimes_oct13.py` compiled **18 ordinary C++ alias/lifetime variants** of the real 98-byte `Print(ASTFunCall*, std::ostream&)` body with GCC 3.3.3. It tested a named callee pointer, AST arguments receiver alias, stream reference/pointer, local AST-call reference, scoped list receiver, and literal locals in both plain `-O2` and register-renaming modes. The best remains **96/98** (offsets 23 and 29, EAX versus original ECX); some callee alias forms regress to 79/98. The previously accepted `Printer_UglyPrint` and its global wrapper remain in the strict suite. This is a *negative* result; do not force register equality via inline assembly or mistake it for progress on a new exact application function.

## Acceptance and full-ELF boundary

Post-batch named-function inventory: **182 EXACT**, **8 EXPERIMENTAL**, **108 UNRESOLVED**, 298 original named function records. The exact suite checks **11,987/11,987 instruction bytes** and all supported original relocations for those candidates; the five new wrappers contribute 155 bytes and five relocations. The accepted named-data suite and BSS gate remain independent of application code. **Whole-ELF byte-for-byte identity is still unavailable**; neither `main`'s linked-file score nor VUC's isolated function counts may be substituted for it.

Reproduce:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-main-rebase-182
```

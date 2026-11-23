# VUC 2004.1 — VCL-style inlining and translation-unit checkpoint

**Date:** 2026-11-23. **Branch:** `vuc` only. PR #23 is intentionally
closed; we will not merge the CMake/VUC integration into `main` before the
**complete rebuilt VUC ELF** matches the original byte-for-byte.

## Reference and baseline

Original VUC 2004.1 ELF32 i386 (314,338 bytes), SHA-256:
`14c77997057f279ab25a3591c6a379d6e4744f2b1551271d43602948c037d8cf`.
Local ZIP SHA-256:
`dacfbeaf4efcde68d45b186c5ed55316fe1a21d1f0aa4350c78580934daf6680`.
These original binaries and the historical toolchains are **not committed**.

Tests used recovered Jan 25 and Dec 6 GCC 3.3.3 prerelease compilers and
GNU as 2.15 in the existing Linux `revcl-oracle` guest. These source-date
candidates are not authenticated as the original Debian GCC 0pre3 binaries.

Independent strict gates on this branch:

- **207/207 exact functions, 16,723/16,723 verified instruction bytes**.
- **36/37 original-length authenticated partial experiments**, matching
  **7,183/11,134 original-offset instruction bytes within those 36 functions**;
  no regressions. `SymbolTable::Enter` is unverified (candidate 194 bytes,
  original 185).
- **244/244 initialized data objects**, **71,536/71,536 matching bytes**.
- **12/12 BSS declarations**, accounting for 18,437 bytes of source
  allocation, **not ELF file-backed bytes**.
- **205/205 host VUC unit tests passed**.
- The original 298 named symbols classify as **207 exact, 37 experimental,
  54 unresolved**, including 6 zero-sized symbols.

See [the complete per-function inventory](FUNCTION_BYTE_MATCH_STATUS.md),
generated from the current original ELF, exact/selected manifests, and the
historical partial-checkpoint report by
`vuc/scripts/render_byte_status.py`. No whole-ELF score is available.

## VCL technique transfer

VCL's historical GCC 3.2.2 reconstruction showed that *genuine*
`static inline` helpers, joining or splitting translation units, and
emission of actual preceding functions can influence register allocation
and post-reload scheduling. This checkpoint tests the mechanism with the
recovered VUC GCC 3.3.3 compiler, not with invented register assignments,
machine-code patches, assembly or artificial padding.

All generated historical objects and result JSON live in temporary guest
directories under `/tmp/vuc-*-20261123/`, not in the repository.

| Target and reproducible driver | Historical builds | Exact comparison result |
| --- | ---: | --- |
| `Print(ASTFunCall*)`: `evaluate_printer_class_layout_20261123.py` | 26 | Thirteen plausible real AST base/member layouts × both compilers all retain **96/98** (mismatches +23,+29); seven relocations and nine literal bytes verified. |
| `AsmOutput_WritePrologue`: `evaluate_prologue_inline_contract_20261123.py` | 34 | Seventeen inline ostream/text helper contracts × both compilers all retain **22/30**, two relocations, 92 literal bytes. |
| `Print(ASTFunCall*)`: `evaluate_printer_inline_owners_20261123.py` | 64 | Sixteen static-inline/member/template helpers × joined/split owner × two compilers. Best **96/98** in 22 original-sized authenticated builds; eight regress to 79/98; two become 101-byte bodies; 32 isolated owners lose literal-section authentication. |
| `Instruction` arena `__tcf_1`: `evaluate_arena_inline_owners_20261123.py` | 54 | Nine real inline cleanup helpers × three source-unit contexts × two compilers. **48** bodies remain **45/46**; six emit invalid 48-byte bodies. The neighboring authentic vector helper remains **265/265** in all 36 joined-source builds; it is absent from the 18 isolated minimal-source objects. No recovery of the original dead `pop ecx`. |
| `Value::operator+`: `evaluate_value_add_inline_owners_20261123.py` | 42 | Seven genuine inline arithmetic helper styles × three joins/splits × two compilers. **36** original-sized bodies remain **203/219**, preserving the TypeConvert relocation and the same 16 mismatch offsets; six become 221-byte bodies. |
| `Bind(ASTFunCall*)`: `evaluate_bind_stack_index_20261123.py` | 42 | Twenty-one real C++03 current-index/pointer-lifetime forms × two compilers. Eighteen retain the original 304-byte length with **300/304 diagnostic code overlap**, 23 verified relocations and 65 referenced literal bytes; 20 become 305 bytes and four 307. This is **not** an accepted partial: 16 compiler-generated `.rodata` alignment bytes remain without authenticated original ownership. |

**262 historical compiler builds total; no newly exact function or improved
authenticated partial score in this batch.** No accepted `vuc/src/`
production code or exact/selected manifest is modified. The experiments
remain reproducible negative controls, not padded or faked matches.

## More promising next steps

1. `Instruction::__tcf_1` requires genuine predecessor/weak-template
   emission state in GCC `peephole2` to recover its remaining dead-register
   byte; the exact 265/265 Instruction vector sibling must remain untouched.
2. The printer's 96/98 callee-load mismatch occurs already at GCC
   `.24.greg`, not in final scheduling. More authentic original
   `Printer.cpp` function bodies and their real register live-range
   interference are more promising than equivalent getters/wrappers.
3. The last 16 bytes of `Value::operator+` are a scalar/x87 case layout
   and scheduling difference, not an arithmetic-lane helper spelling issue.
4. Recovering the original Debian GCC 3.3.3 `0pre3` patches/headers,
   original source-unit ordering and final link environment is important
   for exact whole-file identity.

Do not equate matching a partial instruction body with a complete ELF match.

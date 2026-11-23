# VUC 2004.1 — byte-exact AsmOutput_WritePrologue by source ordering

Date: 2026-11-23. Work stays on vuc until the complete VUC ELF matches.

## Exact match

The original AsmOutput_WritePrologue(std::ofstream&) at 0x08049b40
contains 30 instruction bytes, two original relocations and a 92-byte
original string literal. The previous authentic partial was 22/30.

Joining the already exact AlignString(const char*,int) implementation
before the genuine AsmOutput_WritePrologue definition in one C++03
translation unit changes GCC's receiver/text argument register allocation.
Under -O2 -frename-registers the result is **30/30**, with both
relocations and all 92 literal bytes independently authenticated.

The same compiled object preserves **AlignString 145/145** (eight
relocations) in both recovered historical GCC 3.3.3 snapshots,
2004-01-25 and 2003-12-06. This is another proven instance of the
source-unit ordering technique previously successful in VCL and
the newly exact MLTempString constructors.

Source: vuc/src/AsmOutputPrologueExactOwner.cpp.
Manifest: vuc/config/asm_output_jan25_probe.json.
Reproducer: vuc/probes/evaluate_asmoutput_alignstring_order_20261123.py.

All generated binaries/experiments are restricted to the Linux guest;
no vendor binaries, original ELF, assembly, forced register assignment,
padding, or artificial compiler calls were committed.

## Linkage limitations

The source owner defines both AlignString and the prologue; AlignString
is already accepted independently from vuc/src/AlignString.cpp.
To avoid duplicate definition/linkage risk it is listed in
VUC_HISTORICAL_MATCH_SOURCES and excluded from modern native CMake
object compilation. Its correctness is proven only for those function
bodies, not the original linked translation unit or overall executable.

An earlier control involving the entire AsmOutputUnit could not be
strictly compared: additional literals in that source unit changed the
candidate .rodata section's ownership. Those candidates were not scored
without authenticating the original complete literal region. The
reduced true AlignString+prologue owner avoided that problem.

## Coverage

- 211 exact function symbols; 17,081/17,081 accepted instruction bytes.
- 33 experimental symbols; 32 original-length authenticated partials.
- 6,857/10,776 matching bytes within the 32 partial functions only.
- 54 unresolved symbols, 298 named text symbols total.
- Whole binary identity is not yet available; PR #23 stays closed.

See FUNCTION_BYTE_MATCH_STATUS.md for the current generated ledger.

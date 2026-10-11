# VUC lexer buffer load: source-order investigation

Date: 2026-10-11. Branch: `vuc`.

## Original evidence

The original `yyFlexLexer::yy_load_buffer_state()` at `0x08061780`
is 39 bytes. It reads the active buffer-state pointer from `this+0x28`,
the character count from buffer `+0x10`, the input stream from buffer
`+0x00`, and the cursor from buffer `+0x08`. It then writes those values
to lexer fields `+0x30`, `+0x20`, `+0x34` and `+0x04`, and the byte
under the cursor to `+0x2c`. The original assembly keeps `this` in ECX
and the buffer-state pointer in callee-saved EBX, with a temporary count
in EDX and a temporary input stream in EAX.

The existing natural C++ reconstruction uses the same field addresses,
but the January 2004 GCC 3.3.3 compiler chooses different scratch
registers. Its baseline emits a 37-byte routine; earlier source forms
with explicitly staged reads can emit 39 bytes but do not reproduce
the original register assignment.

## Reproducible ordinary-C++ search

`vuc/probes/evaluate_yy_load_interleavings.py` enumerates load/store
orders that preserve dependencies and the five field-write operations.
It compiles every source form under the recovered January GCC 3.3.3,
GNU as 2.15 and `-O2 -frename-registers`, then checks the original
function through `vuc/scripts/match_probe.py`. Generated C++ and objects
stay in the Lima VM under `/tmp/vuc-binutils215/yy-load-interleavings/`.
The script does not change accepted source or manifests.

The 15 legal load/store orders yielded **12 original-length, 39-byte
candidates**.
None was exact. The best matched **26/39 original bytes**, while three
others matched **24/39**. These were worse than the original 39-byte
register-allocation goal; arranging reads and writes independently does
not yet reproduce the original ECX/EBX state lifetime.

Reproduce:

```sh
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/probes/evaluate_yy_load_interleavings.py
```

## Additional confirmations

- `Printer_UglyPrint()` remains **38/40**, with the `-frename-registers`
  candidate choosing ECX for the literal where the original uses EAX.
- `Canonize2_Reduce()` remains **83/85**, with the candidate choosing
  EDX for the initial status-field load/test where the original uses EAX.
- The original `Value.cpp` iostream initializer wrapper at `0x0806f9d0`
  remains **29/31** using a natural C++ source fragment containing the
  two already accepted `Value` member implementations and `<iostream>`.
  The original body uses EDX for the initializer bound; the candidate
  differs in two register-encoding bytes. The underlying 66-byte
  initializer and 16-byte cleanup callback are already independently
  exact through other accepted manifests, not newly recovered here.

All results are experimental diagnostics. No new function has entered
`vuc/src/` or `vuc/config/exact_candidates.json` in this investigation.
Reconstruction must still reproduce the **entire original VUC ELF**
byte for byte; a near match, even at 38/40, is not accepted.

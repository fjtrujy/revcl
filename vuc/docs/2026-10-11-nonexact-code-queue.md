# VUC original-address nonexact work queue and compiler-hypothesis checks

Date: 2026-10-11. Active branch: `vuc`.

## Exact acceptance remains mandatory

The canonical VUC ELF inside `X86linux_vuc2004_1.zip` is
the sole original-byte authority. During this investigation the
current accepted manifest advanced from **156 to 162** named function
records through a separate committed comparison-operator batch,
`e51693f`. The complete historical GCC 3.3.3 (20040125) and GNU
as 2.15 strict gate was independently rerun against current HEAD:
**162/162 fully exact function symbols, 10,927/10,927 matching code
bytes and 409 resolved relocations**. The new work-queue script
does **not** grant exact status or alter accepted C++ source.

The original text-function ledger now has **162 EXACT**, **9 EXPERIMENTAL**
and **127 UNRESOLVED** symbols out of 298. These numbers and all
original addresses are metadata for investigating *named* functions,
not a binary-wide percentage or indication of ELF byte identity.

## New reproducible work queue

`vuc/scripts/nonexact_work_queue.py` builds its results directly
from the SHA-256-authenticated original ZIP, the exact and
experimental candidate manifests, and the original ELF local
source-file provenance records in `vuc/config/source_units.tsv`.

It validates uniqueness of original function addresses and rejects
overlapping positive-size named-function spans, including those crossing
an exact-function boundary. It reports both short
original functions and larger symbol-order runs bounded by exact functions,
providing a usable implementation order. Unattributed gaps between named
functions are reported separately, never as verified instruction bytes.
Compiler-generated startup symbol names and glibc startup helper
names are filtered from a separate application-oriented shortlist,
but **never** from the canonical original function counts.

Source-file ownership appears only where a local original
`STT_FUNC` has explicit provenance; the tool does not infer
global function ownership from address range. The symbol-byte
counts deliberately exclude zero-length symbols and do not
represent verified executable matching or unknown nonfunction bytes.

Run:

```sh
python3 vuc/scripts/nonexact_work_queue.py --top 15
python3 vuc/scripts/nonexact_work_queue.py --top 50 --json /tmp/vuc-open-code.json
python3 -m unittest discover -s vuc/tests -q
```

Current original-address shortlist (non-startup):

| Original address | Named function | Status and evidence |
| --- | --- | --- |
| `0x08049b40` | `AsmOutput_WritePrologue()` | 30 bytes, experimental 22/30 |
| `0x08061780` | `yyFlexLexer::yy_load_buffer_state()` | 39 bytes, register-allocation mismatch |
| `0x0805cb40` | `Printer_UglyPrint()` | 40 bytes, experimental 38/40 |
| `0x0805a570` | `IsAccumulatorable()` | 77 bytes, normal C++ 61/77 best tested |
| `0x0804ff50` | `Canonize2_Reduce()` | 85 bytes, normal C++ 83/85 best tested |
| `0x0805b770` | `Print(ASTFunCall*)` | 98 bytes, 96/98 with all literals and relocations |
| `0x08071c20` | `MLTempString::MLTempString()` C2 | 101 bytes, 95/101 |
| `0x08071810` | `MLGameStringTable::Get()` | 106 bytes, 94/106 |

These are *not* accepted results. A reconstruction with 99 matching
bytes out of 100 still fails the byte-exact acceptance requirement.

## Experimental compiler results

- 34 historical GCC optimizer and architecture flag variations of the
  `_GLOBAL__I_matchStack` candidate either retained **29/31**
  bytes or changed the emitted length. The original uses `EDX`
  while common compiler hypotheses use `EAX` for the immediate.
- 28 flag variations of the `Instruction` arena teardown kept
  **45/46** matching bytes or changed the function length. The
  remaining byte encodes the scratch register used for the
  epilogue stack restoration.
- An extended historical assembler alignment sweep did not
  reproduce `IsAccumulatorable()` exactly. The 77-byte candidate
  matches 61 bytes; the original aligns an early branch target at
  offset `0x20` and the shared success block at `0x30`,
  while preserving unaligned later handlers. Merely disabling
  jump alignment redistributes padding instead of recovering the
  authentic original block layout.
- Ten idiomatic C++ source forms of the default `MLTempString`
  constructor, each tried under four compiler variants, retained
  **95/101** matching bytes for C2. Its accepted C1 variant
  remains **102/102**, and is not replaced by these probes.
- Historical `std::__uninitialized_copy_aux` of a 513-bucket
  `SymbolScope` still has a larger compiler length than the
  original 120-byte function under the tested alignment settings.
  It remains an unaccepted source hypothesis.

No scratch-specific assembly, fake ELF addresses, forced register
bindings, fabricated instructions, or artificial padding were
introduced. Scratch compiler outputs are outside the repository,
under the existing VM's `/tmp/vuc-binutils215` directory.

## Further work

Prioritize recovering actual C++ control-flow and source layout,
historical Debian `gcc-3.3` package differences, and original
linkage (particularly STL and lexical scanner code). Additional
accepted function records must each pass the strict full gate.
The final success criterion remains **the entire reconstructed VUC
ELF identical to the original file byte for byte**, which is
still unachieved.

# VUC allocator operator new: two byte-exact functions

Date: 2026-10-10. Branch: `vuc`.

The existing interrupted-session fragments `vuc/src/ASTNodeArena.cpp` and
`vuc/src/InstructionArena.cpp` now reproduce both original allocation
methods with the historical GCC 3.3.3 snapshot and GNU as 2.15:

| Original symbol | Address | Byte match |
| --- | --- | --- |
| `ASTNode::operator new(unsigned)` | `0x0804a880` | **86/86** |
| `Instruction::operator new(unsigned)` | `0x08051910` | **86/86** |

These two functions allocate from chained arena blocks of 64 KiB
(`0x10008` bytes including metadata) and 8 KiB (`0x2008` bytes
including metadata) respectively. Each chunk stores the cursor
at its beginning and the previous chunk at offset `0x10004`
or `0x2004`. The code compares the cursor plus requested allocation
size with the chunk boundary and either allocates a new chunk with
global `operator new` or advances the existing cursor.

The checker resolves four new original relocations: the respective
original named BSS arena pointers `ASTNode::mem` at `0x0808abf8`
and `Instruction::instrData` at `0x0808bc78`, and the global
allocation-operator targets. Both named original BSS object symbols
are checked separately against the canonical ELF metadata; the
provisional source names are not just guessed section bases.

The strict selected-function gate now passes **114/114**, **5,896/5,896
original instruction bytes**, **247 resolved relocations**, and
35 per-probe literal bytes. The 50 host tests pass. The exhaustive
original text-function ledger is **114 EXACT, 8 EXPERIMENTAL,
176 UNRESOLVED** of 298 symbol records. It is *not* a whole-binary
match ratio.

The AST arena's per-unit cleanup callback at `0x0804a8f0`
had already been accepted. The Instruction arena cleanup callback
at `0x08051980` remains experimental at **45/46 bytes**.

Reproduce with `python3 -m unittest discover -s vuc/tests -q`,
then `limactl shell revcl-oracle -- python3
/path/to/revcl/vuc/scripts/match_candidates.py
--compiler /tmp/vuc-binutils215/g++-jan25-as215
--out /tmp/vuc-binutils215/exact-gate-114`.

**Byte-for-byte equality of the complete original ELF remains mandatory
and has not been achieved.** The matched functions were not fabricated
as assembly blobs or manually padded.

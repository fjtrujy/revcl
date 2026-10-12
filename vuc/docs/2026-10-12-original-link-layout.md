# VUC canonical linker layout and dynamic relocation contract

Date: 2026-10-12. Branch: `vuc`.

## Why a separate linker contract is necessary

The historical GCC/assembler gate verifies selected named original
function bodies. The separate initialized-data gate verifies selected
named original ELF objects. Those proofs are necessary but **do not**
verify any candidate's linked ELF headers, virtual addresses,
program headers, section layout, compiler-generated padding, dynamic
linking entries or complete file identity.

The read-only `original_link_layout.py` script now extracts an
authenticated **reference link layout** from the SHA-256-validated
ZIP-backed original ELF. It writes deterministic numerical metadata
to `vuc/config/original_link_layout.json`. No binary blob, copied
instructions or reconstructed executable is produced.

## Independently extracted original constraints

| Original ELF property | Value |
| --- | ---: |
| File length | 314,338 bytes |
| ELF class / type / ISA | ELF32 / ET_EXEC / i386 |
| Entrypoint | `0x08049a20` |
| Program headers | 8 |
| Section headers | 35 |
| Dynamic relocation records | 82 |
| `R_386_JMP_SLOT` in `.rel.plt` | 67 |
| `R_386_COPY` in `.rel.dyn` | 14 |
| `R_386_GLOB_DAT` in `.rel.dyn` | 1 |

Both original `PT_LOAD` segments have 4 KiB alignment and non-identical
file-vs-memory extents:

| Segment | File extent | Virtual memory extent | Flags |
| --- | --- | --- | --- |
| 2 | `0x000000..0x02eee4` | `0x08048000..0x08076ee4` | R/X |
| 3 | `0x02f000..0x042784` | `0x08077000..0x0808f4e4` | R/W |

The original `.text` starts at virtual `0x08049a20`,
file offset `0x1a20`, and occupies **165,440 bytes**.
The original `.rodata` starts at virtual `0x08072080`,
file offset `0x2a080`, and occupies **18,560 bytes**.

The original `.bss` is 19,780 allocated bytes, beginning at
`0x0808a7a0`. It is `SHT_NOBITS`, so its section header's
`sh_offset = 0x427a0` **does not designate an additional payload**
in the ELF file. The loader allocates its memory separately.
The writable load segment's file-backed extent ends at `0x42784`,
and there is an intervening 28-byte virtual alignment gap before
the BSS start. These are original *linker requirements*; neither
selective BSS source validation nor copying original section addresses
into C++ would prove that a rebuilt binary reproduces them.

The executable load segment includes six unassigned file spans
between its file-backed named sections. Their exact file positions
and sizes are emitted in the reference JSON. They can contain ELF
headers, program headers, alignment or non-section payloads and
**cannot** be credited as matching instruction/data bytes. The
writable load segment currently has no such spans in the original
section-to-file mapping.

Every original dynamic relocation is recorded with its original
target virtual address, i386 relocation type, dynamic symbol index,
and dynamic symbol name. The parser verifies the dynamic-symbol
table indices, relocation targets inside loadable memory, ELF32
alignment rules, and the relationship between file offsets and
virtual addresses for each file-backed allocated section. It rejects
corrupt layouts rather than guessing a repaired mapping.

## Mandatory distinction

`original_link_layout.json` is a **reference layout**, generated
from the known original. It is not a new exact-match score or
evidence of a candidate. The tool explicitly reports:

```json
{
  "linked_reconstruction_verified": false,
  "whole_elf_byte_exact": false,
  "matching_executable_file_bytes": null
}
```

The existing `whole_elf_gate.py` remains the final acceptance
boundary: a newly built candidate ELF must compare byte-for-byte
against the original original ZIP member, including all 314,338
bytes. No such candidate currently exists.

## Reproduce and verify

```sh
python3 vuc/scripts/original_link_layout.py --summary
python3 vuc/scripts/original_link_layout.py --check
python3 -m unittest discover -s vuc/tests -q
```

Only intentional updates to the canonical source should regenerate
the committed reference:

```sh
python3 vuc/scripts/original_link_layout.py --write
```

The tests independently verify the original loader geometry,
section-backed versus `.bss` bytes, dynamic-relocation type counts,
original symbol names, and rejection of malformed program headers,
misaligned load segments, modified section addresses, and invalid
relocation symbol indices/target addresses.

## Next work

Use the reference to guide original binutils/linker-script
reconstruction, object-file ordering, GOT/PLT layout and runtime
startup. A future candidate must first be a normally produced
historical ELF; comparing a file of copied original bytes would
not establish a source-level reconstruction. Until then all
candidate-level layout and whole-executable match claims remain
unavailable.

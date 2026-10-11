# Byte-exact VUC instruction arena startup lifecycle

Date: 2026-10-11. Branch: `vuc`.

## Reconstructed original code

The historical VUC ELF uses a 0x2008-byte initial allocation for
`Instruction::instrData`, containing an arena cursor in the initial
four bytes and a linked-list predecessor slot at offset `0x2004`.
A normal C++ static object in `vuc/src/InstructionArenaLifecycle.cpp`
initializes that arena and registers its cleanup using original
GCC 3.3.3 C++ static-initialization conventions.

Two newly accepted original functions are byte-exact:

| Original symbol | Address | Verified bytes |
| --- | --- | --- |
| `__static_initialization_and_destruction_0(int,int)` | `0x080514a0` | 138/138 |
| `_GLOBAL__I__ZN11Instruction9instrDataE` | `0x080519b0` | 31/31 |

The 138-byte helper verifies **10 linked relocations** including
the original `_ZSt8__ioinit` at `0x0808bc74`, the original
`Instruction::instrData` at `0x0808bc78`, two local destruction
callbacks, original `operator new`, `__dso_handle` and
`__cxa_atexit`. The 31-byte keyed wrapper verifies a further
original helper-call relocation.

The decisive source-level recovery was storing the initial
`chunk + 4` cursor **before** clearing the `chunk + 0x2004`
predecessor. The reverse order was semantically equivalent but
GCC emitted 118/138 exact bytes; the corrected order yields
138/138 without handwritten assembly or padding.

## Explicitly excluded teardown

The accompanying 46-byte compiler-generated `__tcf_1` at
`0x08051980` still differs in one epilogue register byte:
the candidate emits `pop %eax`, the original `pop %ecx`.
Both BSS/object relocations and the original `operator delete`
call resolve correctly, but 45/46 bytes is not exact, so this
callback remains outside the acceptance manifest. The provisional
C++ lifecycle source is retained because it is needed to emit the
exact static initializer and keyed wrapper, but this does **not**
authenticate the complete original translation unit.

The previously reconstructed AST arena `__tcf_1` and
`main()` are **already** accepted in the historical manifest;
they were rechecked, not newly counted.

## Strict accepted-function gate

The next full gate requires **150/150 accepted function records**,
**8,637/8,637 instruction bytes**, and **388 resolved relocations**.
The inventory remains 298 original function symbols:
**150 EXACT, 10 EXPERIMENTAL, 138 UNRESOLVED**. This is a
function-record ledger, not an executable-wide match percentage.

Reproduce with the 2004-01-25 source-date GCC 3.3.3 and GNU as 2.15:

```sh
python3 -m unittest discover -s vuc/tests -q
python3 vuc/scripts/coverage.py --write
limactl shell revcl-oracle -- python3 \
  /path/to/revcl/vuc/scripts/match_candidates.py \
  --compiler /tmp/vuc-binutils215/g++-jan25-as215 \
  --out /tmp/vuc-binutils215/exact-gate-150
```

The final objective remains **whole-ELF byte identity**, including
all remaining code/data/section contents/linker metadata. No
complete original-sized reconstructed executable has passed the
whole-ELF gate.

# VUC Bind(ASTFunCall*) literal and linker ownership audit

Date: 2026-10-16. Original target: the canonical unchanged VUC 2004.1
Linux i386 ELF32. The existing original-sized source candidate
`vuc/probes/BindASTFunCallFreshOct15r.cpp` has a **300/304
instruction-byte diagnostic overlap**, with 23 original relocation
sites. This score is **not** an eligible EXPERIMENTAL match because the
candidate's entire compiler-produced literal section has not been
authenticated; the exact and selected manifests remain unchanged.

## Recovered original cross-references

The reproducible read-only audit
`vuc/probes/audit_bind_literal_owner_oct16.py` reads the canonical
archived executable through `vuc/scripts/decomp.py`, verifies exact
literal bytes, and scans original `.text` for little-endian immediate
addresses. The following actual address references were established:

| Original literal | Address | Original code references | User |
| --- | --- | --- | --- |
| `"main"\0` | `0x08072301` | `0x0804ea05` | `Binder_Bind(ASTModule*)` |
| `"const"\0` | `0x08072331` | `0x0804f309`, `0x0804f3b5` | `operator>>(std::istream&, Type&)` |
| `"' uses recursion! Recursion is forbidden in VUC!"\0` | `0x08072340` | `0x0804abb2` | `Bind(ASTFunCall*, SymbolTable&)` |

The original bytes at `0x08072336..0x0807233f` are exactly **10
alignment zeros**. They occur *after* the six-byte
`"const"\0` string and immediately before the 32-byte-aligned
recursion diagnostic.

The current 304-byte Bind candidate independently reproduces all
three of its referenced strings and 23 code relocations, but its
single-function `.rodata` has **16 zeros** at offsets 16..31
before the long diagnostic. The original's 10-byte gap is not the
same object-local sequence: original `"const"\0` is used by an
entirely different named function. We cannot map six of the
candidate's 16 zeros to the original six nonzero `"const"\0`
bytes or to unrelated zero bytes merely to satisfy a checker.

This is evidence about **linked literal ordering and cross-function
ownership**, not proof that the two methods came from one original
translation unit. The `Binder.cpp` local-function metadata and the
original address order are useful constraints, but neither alone
authenticates the source-unit merger. Do not introduce an unused
`"const"` string, dummy function, compiler padding trick, or
handwritten bytes solely to fill the gap.

## Reproduction and next discriminator

```sh
python3 vuc/probes/audit_bind_literal_owner_oct16.py
```

The script asserts all three literal contents, all listed original
immediate references, and the authentic ten-byte gap. It does not
run the compiler, modify the original archive, infer relocations
from arbitrary zero runs, or adjust the production matcher.

To make `Bind(ASTFunCall*)` a strict experimental candidate, recover
an authentic original compilation-unit/literal-emission context that
explains the exact object section, or establish a genuinely
ELF-supported alternative source layout. Then compile with both
historical GCC 3.3.3 snapshots and require **304 original-length
bytes, 23/23 authentic relocations, all literal bytes and entire
object-literal section accounted for**. Until then, **300/304 remains
diagnostic**, not accepted byte-exact or partial coverage.

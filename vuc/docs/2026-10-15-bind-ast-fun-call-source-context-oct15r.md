# VUC Bind(ASTFunCall*): new 300/304 diagnostic, not yet strict eligible

Date: 2026-10-15. ELF32 VUC 2004.1 i386.
Original function: `Bind(ASTFunCall*,SymbolTable&)` at
`0x0804aad0`, **304 bytes**. It was previously UNRESOLVED.

## Source and behavioral recovery

`vuc/probes/BindASTFunCallFreshOct15r.cpp` is a complete
plausible old-C++03 source reconstruction from independent original
disassembly and already-accepted neighboring methods:

1. Bind the callee `ASTLValExpr*` at ASTFunCall+8 and the expression
   argument list at ASTFunCall+12.
2. Compare the callee's bound `SymbolTableEntry*` at ASTLValExpr+32
   with `SymbolTable::Find(enclosingFunID,false)` and return if different.
3. If recursive, resolve the name by calling
   `call->callee->entry->name.c_str()`, where
   `SymbolTableEntry::name` resides at offset +32.
4. Set original global `error` at `0x0808bccc`, print
   `currFileName`, then **start a fresh std::cout expression**
   to print `"("`, `matchStack[matchStack.length-1]->line`,
   `" ): function '" (without the space), function name,
   the original recursion-forbidden diagnostic and `std::endl`.

The full source uses the original 12-byte `ASTNodeList` layout,
whose first word is **signed length** and whose third word at
offset +8 is an array of `ASTNode*`. This corrects a misleading
earlier provisional `AST.cpp` interpretation that the BSS
`matchStack` had three vector-pointer fields. The byte-exact
global constructor is not modified: its three zero stores alone
cannot distinguish either representation.

The original signed `ASTNode::line` is located at +4. GCC 3.3.3
inlines the C++03 `std::ostream::operator<<(int)` behavior into
a runtime branch that selects `operator<<(unsigned long)` vs.
`operator<<(long)` depending on stream basefield flags. Declaring
the field unsigned removes that original branch and shortens
the function substantially.

Similarly, a single chained
`std::cout << currFileName << "("` produces **300 bytes**:
GCC reuses the returned stream pointer. The original disassembly
has a separate `movl $std::cout,(%esp)`, so faithfully splitting
the two print statements grows the candidate to **304 bytes**,
the **original function length**. Both recovered historical GCC
3.3.3 snapshots reproduce this result with `-O2
-frename-registers`.

## Diagnostic code-byte comparison: 300/304

With all **23 original relocation sites** resolved to their
authenticated original ELF named callees, globals and source
literals, the original-length 304-byte function differs at
only **four byte offsets**, +178, +179, +180 and +181:

- Original emits `decl %eax` then indexed array access
  `mov (%esi,%eax,4),%edi`;
- Candidate fuses the decrement into a memory displacement,
  `mov -4(%esi,%eax,4),%edi`.

The generated `.rodata` section contains three **genuinely
authenticated** original strings, mapped as:

| Object bytes | Source string | Original address |
| --- | --- | --- |
| 0..1 | `"("\0` | `0x08075c5d` |
| 2..15 | `" ): function '"\0` without initial space | `0x08072278` |
| 32..80 | `"' uses recursion! Recursion is forbidden in VUC!"\0` | `0x08072340` |

**Crucial strict acceptance blocker:** The compiler object includes
**16 additional zero bytes** at literal-section offsets 16..31,
emitted as alignment padding before the long diagnostic string.
The authentic original linked ELF has only 10 adjacent padding
zeros in its distinct combined source-unit literal context at
`0x08072336..0x0807233f`. Arbitrarily mapping the extra zero
padding to unrelated original ELF zero bytes would fake
literal ownership and is **not permitted**.

Therefore the strict `match_probe.py` rejects the candidate
with `candidate literal section contains unmapped bytes`.
The **300/304 instruction comparison is explicitly diagnostic
only**, obtained with `prepare_named_function` after each
*referenced* candidate literal range is independently verified
byte-identical against the original ELF; it bypasses no
acceptance gate and must never appear in
`vuc/config/selected_candidates.json` or
`vuc/config/exact_candidates.json` yet.
The near-match is neither EXACT nor EXPERIMENTAL in the official
inventory until authentic entire literal-section ownership
can be reconstructed.

The companion bounded control
`vuc/probes/evaluate_bind_ast_fun_call_index_oct15r.py`
tests 12 genuinely plausible C++03 source indexing idioms
with both compiler snapshots. Direct indexing, pointer
arithmetic, different signed/unsigned casts and `length+-1`
all naturally produce the same **304-byte length, 23 relocation
sites, 300/304 diagnostic code bytes**; helper-based forms
compile to different lengths or external calls. None resolves
the four bytes. Generated source/object files and full JSON
ledger are stored under
`/tmp/vuc-binutils215/bind-ast-index-oct15r/` in the existing
historical Lima guest.

## Follow-up

Reconstruct preceding original `Bind.cpp` literal/translation-unit
context and the original `ASTNodeList` last-index access helper.
Recover actual original `.rodata` padding from genuine TU
predecessors rather than assigning anonymous zero bytes to
a coincidental region. Retain the exact-candidate rules.
No whole-ELF byte-exact match is established.

As of this note, accepted coverage is unchanged:
**204 EXACT / 34 EXPERIMENTAL / 60 UNRESOLVED**.

## Independent neighboring IR2Asm.cpp and Translate.cpp owner control

Before investigating Bind, this batch tested whether real
`IR2AsmBroadcastUnit.cpp` and `TranslateTypeUnit.cpp` source
context—not synthetic predecessor instructions—could change the
previously selected 416-byte `BroadcastMove` and 352-byte
`Translate(Type)` partials. Reproduction script:
`vuc/probes/evaluate_ir2asm_translate_owner_context_oct15r.py`.

With both historical GCC snapshots, compiler flags as originally
selected, and the real accepted unit's iostream initializer and
class declarations, `BroadcastMove` remained **191/416** with
all 27 relocations and 47 literal bytes, and `Translate(Type)`
remained **64/352** with all 12 relocations. The genuine startup
owner compiler flag set expanded `BroadcastMove` to **424 bytes**
(strict size rejection), while `Translate(Type)` stayed at 352.

This establishes that the real iostream initialization predecessor
alone does not explain these two mismatches, unlike the earlier
FlexLexer predecessor match. Do not repeat pure include/preamble
variations without new source-unit evidence. Their entire score
ledger and historical ELF32 objects are preserved at
`/tmp/vuc-binutils215/ir2asm-translate-owner-oct15r/`.

# VUC: byte-exact string-table insertion after branch rename

Date: 2026-10-14. Branch: `vuc` (renamed from
`vuc-unresolved-oct14` by the user). Starting rebased commit:
`d6b698e`, on top of VCL `origin/main` at `8ce5834`.
The 146 pre-existing unrelated untracked paths and the unfinished
statement-list source-sweep script were preserved.

## New 263/263 exact function

**`MLGameStringTable::Add(const char *, bool)`**
(`_ZN17MLGameStringTable3AddEPKcb`) starts at original address
**`0x08071880`**, with exactly **263 original instruction bytes**.

Accepted source: `vuc/src/MLGameStringTableAdd.cpp`.
Strict manifest: `vuc/config/string_table_add_oct14_probe.json`.
Historical compiler: January 25, 2004 GCC 3.3.3 prerelease, using the
working original-era GNU as 2.15 wrapper
`/tmp/vuc-binutils215/g++-jan25-as215`. Accepted flags:
`-O2 -frename-registers`.

The independent ELF relocation matcher confirms:

- 263/263 matching compiled instruction bytes, `exact: true`;
- all 10 original relocation sites, covering eight distinct
  relocation targets;
- real `FindCopy`, C-string hash, `strlen`, `strcpy`,
  scalar `new`, array `new[]`, `delete`, and `_Unwind_Resume`;
- no unauthenticated candidate string literals.

Original-data structure and control flow recovered:

1. Check the 513-bucket string interning table for the input text. A
   cached `MLGameStringData*` is returned immediately if present.
2. Otherwise allocate a **16-byte** `MLGameStringData` record.
   Record the `temporary` byte at +12, string length at +8, and
   the genuine `MLHashFun<const char*>::Hash` result at +0.
   The string field is at +4: it either aliases the input for a
   temporary string or owns a `new char[length+1]` copy.
3. Allocate a **16-byte** hash node with key at +0, hash at +4,
   value at +8, and next pointer at +12. The hash is evaluated
   inside the inlined C++ node constructor, after allocation, with
   compiler-generated `operator delete` / unwind cleanup if the
   constructor throws.
4. Compute the bucket using the original modulo-513 reciprocal
   multiplication and insert the node at the bucket head.

### The decisive source-language difference

The initial natural source
`vuc/probes/string_table_add_source_oct14.cpp` emitted **269**
bytes, six more than the original; its argument source/lifetime
differed around the hash-node allocation.

The exact original-era source uses this **ordinary C++03 reference
to an existing member**, immediately before constructing the node:

```cpp
const char *const &name = data->text;
Table::Node *node = new Table::Node(name, data);
```

Compared to using `new Table::Node(data->text, data)` directly,
the named reference makes GCC materialize the address of
`data->text` **before** calling scalar `operator new`.
This recovers the original +0x7d `lea` and the associated
register lifetimes. It is a genuine expression preserving the
original function's C++ semantics, not a register-binding asm
statement or a patched instruction stream.

`vuc/probes/evaluate_string_table_node_lifetime_oct14.py`
reproduces the discovery from source variants, flags and the strict
original ELF matcher. The large bounded source-control evaluator
`vuc/probes/evaluate_string_table_add_oct14.py` tests alternative
constructor order, data and node ownership, bucket-address aliases,
and historical compilation flags. The former found
`NEW_EXACT const-ref r`; compiling the committed accepted source
independently reproduces the full match.

### Reproduce the committed source

```sh
R=/path/to/revcl/vuc
C=/tmp/vuc-binutils215/g++-jan25-as215
limactl shell revcl-oracle -- "$C" -O2 -frename-registers \
  -c "$R/src/MLGameStringTableAdd.cpp" \
  -o /tmp/vuc-binutils215/string-table-add-accepted-oct14.o
limactl shell revcl-oracle -- python3 "$R/scripts/match_probe.py" \
  --object /tmp/vuc-binutils215/string-table-add-accepted-oct14.o \
  --config "$R/config/string_table_add_oct14_probe.json"
```

All compiler scratch objects remain under `/tmp/vuc-binutils215`
in the existing Lima VM. Original ZIP/ELF and root VCL sources
remain untouched.

## Other small-function control

The prior interrupted session left the untracked file
`vuc/probes/evaluate_stmt_fold_source_oct14.py`. It has now
been validated and preserved as negative evidence. Its 132
source-form/compiler combinations exercise a genuine
`Translate(ASTNodeList<ASTStmt*>&)` fold (original
`0x0806ada0`, 119 bytes): signed/unsigned loop index,
early-continue / goto semantics, result aliases and lifetime
variants, as well as GCC block reordering and alignment.
None authenticates a 119-byte candidate. Do not promote a
116- or 130-byte semantic match on the basis of matching behavior.

## Acceptance and remaining limits

The strict acceptance manifest gains the accepted source above,
and the status inventory is now **196 EXACT / 15 EXPERIMENTAL /
87 UNRESOLVED** out of 298 named original functions. The
accepted instruction-byte total becomes **14,255** (previously
13,992). The independent named-data and BSS verification gates
remain mandatory. This new function does **not** prove whole
linked-ELF byte identity; source-unit boundaries and linker
layout are still separate work.

The next useful targets are a more exact original `ASTNodeList::Add`
implementation across template instantiations, or the near-matching
smaller nonexact constructors and helper functions. Preserve the
historical source-unit caveat: VCL GCC 3.2.2 compiler tricks are
hypotheses, not automatic truths about VUC GCC 3.3.3.

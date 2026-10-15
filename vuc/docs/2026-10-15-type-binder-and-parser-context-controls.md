# VUC C++03 owner-context controls: Type insert, Binder, Scope, Bind and ParseArguments

Date: 2026-10-15. Canonical original: VUC 2004.1 ELF32 Linux i386.

## Why the accepted Type vector candidate stays at 481/525

The selected candidate remains `vuc/probes/TypeVectorInsertAuxRealOrderOct15s.cpp`,
compiled with original-order GNU C++03 explicit template instantiations.
The verified original-length **525 bytes**, **481 matching instruction
bytes**, **9 relocations**, and two accepted neighboring iterator-copy
functions (**79/79** and **86/86**) are unchanged.

A direct original-offset normalized instruction comparison localizes
the 44 residual bytes to original function-relative offsets:

- +0x40..+0x5e: temporary word-register/scheduling order while copying
  the 16-byte Type value before moving array entries backward;
- +0xd5,+0xd8: scratch register choice for the vector growth branch;
- +0x10d..+0x11d: interleaved 16-byte Type move vs. GCC's alternate
  original-length scheduling;
- +0x178..+0x185 and +0x1a0..+0x1a6: later allocation/cleanup
  scratch-register selection.

The authentic relocations and final size already match; these are
source/translation-unit ownership questions, **not** valid reasons to
insert bytes or rewrite relocation targets. Previous genuine
copy-constructor / copy-assignment forms are documented separately
in `2026-10-15-type-vector-template-order-and-negative-controls.md`.
No Type source was changed in this round.

## Binder_Bind(ASTModule*) authentic lifetime controls

Original: `0x0804e980`, **552 bytes**. Source:
`vuc/probes/BinderBindASTModuleFreshOct15s.cpp`. The candidate
already reproduces Scope begin/end calls, the three module list
bindings, temporary 100-entry vector reserve, `main` lookup,
export flag, 0x804-byte Scope destruction and allocation cleanup
in the original order.

Two new bounded source hypothesis suites test actual historical C++03
class and scope ownership rather than unrelated compiler switches:

- `vuc/probes/evaluate_binder_scope_lifetime_oct15t.py`:
  genuine `MLHashTable` destructor/clear exception declarations,
  local SymbolTable references, a default-initialized temporary,
  and ordinary early/predicate spellings.
- `vuc/probes/evaluate_binder_symboltable_ownership_oct15t.py`:
  `std::vector<Scope>` as a direct SymbolTable member or base class,
  user-authored empty/non-throwing destructors, local references,
  member initialization and constructor/reserve spelling.

With both historical GCC 3.3.3 snapshots, unchanged source usually
emits **537 bytes** with the four-byte jumps control or **557 bytes**
with eight-byte jumps, the closest known candidate remaining five
bytes above original. `~MLHashTable() throw()` expands the original
eight-jump code to **604 bytes**; `clear() throw()` collapses it to
**466 bytes** by substantially changing exceptional cleanup.
A real but unlikely historical `if(this) clear()` bug yields
**533 bytes**, not an exact-length candidate. Inheriting instead
of containing `std::vector<Scope>` still yields **557 bytes**.
No source variant is eligible for the selected manifest.

## Bind(ASTFunCall*) actual list-owner context

The existing `vuc/probes/BindASTFunCallFreshOct15r.cpp` matches
**300/304 code bytes diagnostically** (23 authentic relocations)
but remains blocked by 16 unowned compiler-generated `.rodata`
alignment bytes. The new genuine-translation-unit experiment
`vuc/probes/evaluate_bind_ast_fun_call_real_owners_oct15t.py`
co-compiles the source with actual accepted `BindLists.cpp`
templated ASTNodeList traversal bodies, testing both relative source
orderings and both historical compiler snapshots.

All six controls reproduce **304 original-length bytes,
300 diagnostic matching bytes, 23 relocations**, and the
same 16 unowned literal-section alignment bytes.
The referenced three original strings were independently checked,
but strict whole-section ownership is still **unproven**.
This diagnostic remains deliberately outside all manifests.

## Scope vector iterator-copy constructor ownership

The unselected `std::__uninitialized_copy_aux<ScopeIter,ScopeIter>`
at `0x0806a840` is **149 original bytes**. A genuine
`ScopeCopyTable.h` declaration with exception-qualified `clear()`
yields **136 compiler bytes** at natural `-O2`.
Changing to the independently used `MLHashTable.h`, where
`clear()` is not declared `throw()`, produces **159 bytes**.
Neither declaration recovers the original 149-byte length. Neither
is selected; the canonical accepted Scope-copy class remains intact.

## Previously untouched ParseArguments: behavioral recovery

`ParseArguments(int, char**)` at **0x0805b3e0**, original
**777 bytes**, was previously UNRESOLVED without a source draft.
The new `vuc/probes/ParseArgumentsFreshOct15t.cpp`
is a conventional historical C++03 source hypothesis, supported
by independent original ELF relocation and disassembly evidence:

- GNU GCC emits the original `rep cmpsb` initial CLI switch check
  for `-comments`/`/comments` and `-c`/`/c`;
- the other original option strings are `reduce`,
  `nowarnings`, and `nw`, controlling BSS `addSourceComments`
  at `0x0808bc89`, `reduceConstantExpressions` at
  `0x0808bc8a`, and `noWarnings` at `0x0808bc8b`;
- the three original 256-byte data arrays are
  `inputFileName` `0x08077860`,
  `outputFileName` `0x08077960`, and
  `inputPath` `0x08077a60`;
- it recognizes a maximum length of **255**, appends or replaces
  a `.vcl` filename suffix, rejects input/output equality,
  and derives the input directory from '/' or '\\';
- the original named error strings include:
  `Error: Output file name too long.\n`,
  `Error: Input and output file the same.\n`,
  `Error: Input file name missing.\n`, and
  `Error: Invalid input argument.\n`.
  The original full help text names Ola Olsson and Peter Pettersson.

**This is a behavioral draft, not a strict or partial byte match.**
The old compiler emits **681 bytes** when C++ source uses
`std::strcat(outputFileName,".vcl")`; replacing it with
`strcpy(outputFileName+strlen(outputFileName),".vcl")`
emits **697 bytes** under both historical compiler snapshots
with `-O2 -frename-registers`, compared to original 777.
The source produces an out-of-line `memcpy` relocation for the
path prefix, while the original ELF emits inlined
`rep movsl` and conditional 1/2-byte tail copies.
This difference matters and cannot be wished away.

The repeatable
`vuc/probes/evaluate_parse_arguments_copy_idioms_oct15t.py`
compares nine actual C++03 prefix-copy expressions:
`memcpy`, `__builtin_memcpy`, `strncpy`,
`std::copy`, manual indexed/pointer loops, and the
`strcat` alternate. All still emit either 681 or 697
bytes under both GCC snapshots. The original-length and
strict original-relocation/literal acceptance gates are not met;
`ParseArguments` remains UNRESOLVED.

## DWARF verification and next steps

The original ELF's `.debug_info` contains **2,696 bytes**.
Examining its actual `DW_TAG_compile_unit` records with the
original-era toolchain confirms GNU assembly startup
(`../sysdeps/i386/elf/start.S`) and GNU C runtime
(`init.c`, GCC 3.3.3) compilation-unit metadata. Searching
for a recoverable application `Type` class definition
does **not** yield its member layout. Accordingly the
current 16-byte `Type` field names are **compiler-compatible
reconstructions, not authenticated original source names**.

The machine-code target remains exact, but the original
whole linked executable is not yet byte-identical. The
official 298-function coverage remains
**204 EXACT / 36 EXPERIMENTAL / 58 UNRESOLVED**.

Best next directions: authenticate real original Type insertion
preceding source methods and their register scheduling, and
recover the genuine C++03 directory-prefix copy in
`ParseArguments`. Only move an original-length, completely
relocation/literal-authenticated candidate into the
experimental inventory; preserve all established exact matches.

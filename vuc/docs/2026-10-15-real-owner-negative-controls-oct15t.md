# VUC C++03 source-owner negative controls: Type, Binder, Scope and Bind

2026-10-15. Historical target: VUC 2004.1 Linux ELF32/i386.
No accepted source or original archives have been modified.

## Type vector insertion: 48 authored field-order controls

Original std::vector<Type>::_M_insert_aux at 0x0804ee10, original
size 525. The selected authentic-order Type TU remains 481/525,
with all nine authentic relocations and with both neighboring
iterator-copy specializations 79/79 and 86/86 exact.

Reproduction: vuc/probes/evaluate_type_vector_authored_field_order_oct15t.py.
The 24 permutations of real C++03 copy-constructor body assignment
order and 24 explicit copy-assignment orderings were each compiled
with January 2004 GCC 3.3.3 and GNU as 2.15. Each was checked
against original length, relocations and both previously exact
iterator-copy functions. The original a,b,c,d constructor and
implicit assignment retains the strongest 481/525 and preserves
79/79, 86/86. The next closest constructor a,c,b,d reaches
479/525, but breaks those accepted iterator-copy functions
to 75/79 and 82/86. Explicit assignment worsens the source
byte score. Do not repeat this grid.
Guest ledger: /tmp/vuc-binutils215/type-vector-field-order-oct15t/.

## Binder_Bind original-name local ownership

Original Binder_Bind(ASTModule*) at 0x0804e980, 552 bytes.
The source already reproduces original local Scope vector
reservation, AST lists, main-name lookup, and exception cleanup,
but emits 557 bytes under the selected historical compiler flags.

Reproduction: vuc/probes/evaluate_binder_main_name_ownership_oct15t.py.
Fourteen real C++03 conditional and local lifetimes were tested
under both recovered GCC 3.3.3 snapshots. All compile to 557 bytes.
Changing the human spelling of mainName, its pointer result or
exportModule predicate alone does not explain the five-byte gap.
Guest ledger: /tmp/vuc-binutils215/binder-real-source-lifetime-oct15t/.

## Authentic SymbolTable.cpp predecessor for 149-byte ScopeIterator copy

Original uninitialized_copy_aux<ScopeIter,ScopeIter> at 0x0806a840,
149 bytes. Isolated historical STL template source emits 136 bytes.
In the original reconstructed SymbolTable translation unit, **after**
the actual SymbolTable::BeginScope and the 746-byte Scope vector
insertion, the real 0x804-byte element stride, placement-new,
loop and compiler catch cleanup emit the following sizes
with both original GCC snapshots:

| Source-unit compiler recipe | Iterator copy | Scope insertion |
| --- | ---: | ---: |
| -O2 -frename-registers -falign-jumps=1 | 145 | 746 |
| -O2 -frename-registers -falign-jumps=4 | 147 | 749 |
| -O2 -frename-registers -falign-jumps=8 | 151 | 756 |

Sources: vuc/probes/ScopeNormalIteratorRealOrderOct15t.cpp and
vuc/probes/ScopeNormalIteratorSymbolTableUnitOct15t.cpp.
Compiling also with the genuine Enter, Find, BeginScope,
EndScope, and FindCopy source does not change these sizes.

The candidate matches the original loop instruction order; the
missing/excess instructions are compiler alignment around the first
exception pad and the rethrow continuation. Do NOT hand-insert
or remove NOP instructions.

The reproducer
vuc/probes/evaluate_scope_iterator_original_table_owner_oct15t.py
tests eight source-level MLHashTable C++03 constructor/destructor/
clear() exception contract declarations, with both recovered GCCs
and three narrowly constrained alignment contexts. Ordinary
destructor spellings are identical; throw() declarations substantially
change compiler EH. No source attains the original 149 bytes while
preserving the neighboring 746-byte insertion. Guest ledger:
 /tmp/vuc-binutils215/scope-iterator-original-table-owner-oct15t/.

This remains UNRESOLVED, never an accepted original-length partial.

## Bind(ASTFunCall*) 304-byte index access

Original at 0x0804aad0, length 304, 23 authentic relocations.
The prior source has a diagnostic comparison of 300/304
original-offset instruction bytes. It cannot qualify for the
experimental or exact manifests: 16 generated .rodata alignment
bytes still lack authentic original translation-unit ownership.

The source
vuc/probes/evaluate_bind_ast_fun_call_true_index_lifetimes_oct15t.py
tests 41 plausible C++03 signed-index, pointer/array-alias,
pre-decrement and named length lifetimes at five points around
the original bind/iostream statements. Each separately validates
all 23 named relocations and exact bytes of the three referenced
original strings before a diagnostic code comparison. Baseline
remains the best 300/304. One other original-length candidate
produces only 209/304; most variants grow to 317-326 bytes.
Do not reuse any diagnostic byte score for formal acceptance.
Guest ledger:
 /tmp/vuc-binutils215/bind-ast-index-lifetime-oct15t/.

## Official status

204 EXACT / 36 EXPERIMENTAL / 58 UNRESOLVED over 298 symbols.
The linked whole ELF is not byte-exact. These independent controls
are retained so future source-unit reconstruction can skip
unsuccessful spellings and move to other untouched functions.

## Narrow after-source-unit EH alignment controls

Following reconstruction of the actual original SymbolTable and
Scope template owner order, a further fixed, small compiler-alignment
control checked only jumps, catch-pad labels and loop labels implicated
by the original disassembly. Reproducer:
vuc/probes/evaluate_scope_iterator_eh_label_controls_oct15t.py.
The results remain incompatible with reproducing **both** the
original 149-byte Scope iterator-copy and the existing original-length
746-byte Scope insertion in one normal historical TU. For example,
the genuine insertion remains 746 with `jumps=1`, but the iterator is
145; `jumps=4` yields 749/147; and `jumps=8` yields 756/151.
Changing loop or label alignment can shorten the iterator further
but also invalidates the original insertion. This is negative
compiler evidence, not authorization to pad source functions.
All source and ELF32 score rows are in
/tmp/vuc-binutils215/scope-iterator-eh-label-controls-oct15t/.

A similarly bounded check on the genuine
std::vector<std::string> insertion + accepted string iterator-copy
TU appears in
vuc/probes/evaluate_string_vector_original_owner_controls_oct15t.py.
Neither historical GCC snapshot produced the **668-byte** original
vector insertion among the tested historical branch/loop/label
alignments. The closest candidate is **672 bytes**, but it also
breaks the original **161-byte** adjacent string iterator-copy,
expanding it to 163 bytes. The 676-byte candidate with exact
161/161 neighboring copy remains four times more distant in length.
Full results: /tmp/vuc-binutils215/string-vector-original-owner-controls-oct15t/.

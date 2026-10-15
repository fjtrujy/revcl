# VUC Type vector: authentic GCC instantiation order and new 481/525 partial

Date: 2026-10-15; canonical VUC 2004.1 Linux ELF32 i386, branch `vuc`.
No original binary files or unrelated untracked work were modified.

## New source-matched untouched application helper

Original symbol:
`std::vector<Type>::_M_insert_aux(__gnu_cxx::__normal_iterator<Type*,std::vector<Type> >,const Type&)`
at **0x0804ee10**, length **525 bytes**.

The original ELF places this function **before** the previously byte-exact
`std::__uninitialized_copy_aux` Type iterator specializations:

1. `_M_insert_aux` at `0x0804ee10`, original 525 bytes;
2. `const Type` iterator-copy at `0x0804f0e0`, 79 bytes EXACT;
3. `Type` iterator-copy at `0x0804f1c0`, 86 bytes EXACT.

The new `vuc/probes/TypeVectorInsertAuxRealOrderOct15s.cpp` is
a **single ordinary C++03 source unit** with those three authentic
historical GCC `std::vector<Type>` instantiations in original order.
It reuses the independently accepted source-level 16-byte `Type`
layout, explicit copy constructor, `TypeIterator` declaration and
the historical GNU libstdc++ 3.3.3 template definitions. No fabricated
calls, inline assembly, byte arrays, literal padding or raw object
edits are present.

With `-O2 -frename-registers` and either original January 2004 or
December 2003 GCC 3.3.3 prerelease snapshot + GNU as 2.15, the single
object strictly reproduces:

| Historical function | Function bytes | Original-offset matches | Relocations |
| --- | ---: | ---: | ---: |
| `TypeVector::_M_insert_aux` | **525/525** | **481/525** | **9/9** |
| `std::__uninitialized_copy_aux<const Type...>` | **79/79** | **79/79** | 0 |
| `std::__uninitialized_copy_aux<Type...>` | **86/86** | **86/86** | 0 |

The source for the first was previously UNRESOLVED and is now
**EXPERIMENTAL** only. All nine original allocation, reallocation,
copy, catch and unwinding relocations are authenticated against
original named ELF function symbols, including two calls to the
previously accepted mutable Type iterator-copy. This function has no
embedded `.rodata` literals. The remaining 44 mismatching
instruction bytes are primarily in the original in-place copy-loop
register choices and around the grow-path branch/exception layout.
No exact match is claimed.

## Real translation-unit ordering is essential

The reproduction scripts
`vuc/probes/evaluate_type_vector_real_ctor_lifetime_oct15s.py`
and
`vuc/probes/evaluate_type_vector_original_instantiation_order_oct15s.py`
independently test the source-unit codegen effect.

Using the accepted Type iterator-copy source **before** instantiating
`_M_insert_aux` produces only **440 bytes**, and therefore fails the
strict original-length gate. Moving the same C++03 `_M_insert_aux`
instantiation before both authentic iterator-copy specializations
reproduces **525 bytes** and **481** original-offset matching bytes.
Both iterator-copy methods remain independently exact **79/79** and
**86/86** in the same object. This replicates on **both GCC snapshots**.

Adding real default constructors, from empty historical
`Type() {}` to value-initializing member lists, did **not** change
the 440-byte copies-first output or damage the two exact functions.
The compiler differences are from **source-unit instantiation
ownership/order**, not an artificial source default constructor or
a modern optimizer assumption.

The individual control source
`vuc/probes/TypeVectorInsertAuxFreshOct15s.cpp` emits a 525-byte
candidate in isolation, with the same strict 481/525 match. The
new selected manifest
`vuc/config/type_vector_insert_aux_partial_oct15s.json` references
the **genuine original-order owner** and remains outside
`exact_candidates.json`. Reproduction:

### Compiler-owner C++03 assignment idioms

After committing the original-order partial, another bounded
reproducible experiment,
`vuc/probes/evaluate_type_vector_copy_assignment_source_oct15s.py`,
tested historically plausible Type copy constructor bodies and explicit
`Type::operator=` declarations. In both GCC snapshots, the
implicit-assignment original-order source still yields **481/525**.
Replacing the copy constructor initializer list with forward body
assignments is equivalent (**481/525**), but a reverse-order body
falls to **446/525** and breaks both accepted iterator-copy functions
(58/79 and 65/86). Explicit forward, reverse, alternating and
pairwise assignment operators retain the original **525-byte length**
and nine relocations but reduce original-offset code matches to
**445, 436, 448 and 442**, respectively. An ordinary 16-byte
`__builtin_memcpy` assignment compiles identically to the selected
**481/525** source.

This confirms the baseline field-copy order and shows that recovering
different named C++03 assignment semantics is not by itself the
remaining register/branch solution. The same two accepted Type
uninitialized-copy instantiations stay exact with all forward-explicit
assignment controls. Objects, source variants and JSON score ledger
remain outside Git under
`/tmp/vuc-binutils215/type-vector-copyassign-oct15s/`.

```sh
R=/path/to/revcl/vuc
/tmp/vuc-binutils215/g++-jan25-as215 -O2 -frename-registers \
  -c "$R/probes/TypeVectorInsertAuxRealOrderOct15s.cpp" \
  -o /tmp/vuc-binutils215/type-vector-real-owner.o
for config in type_vector_insert_aux_partial_oct15s \
              uninitialized_type_const_probe \
              uninitialized_type_mut_probe; do
  python3 "$R/scripts/match_probe.py" \
    --object /tmp/vuc-binutils215/type-vector-real-owner.o \
    --config "$R/config/${config}.json"
done
```

Reproducible compiler objects, genuine-source controls and JSON result
ledgers remain outside Git in the existing Linux guest under
`/tmp/vuc-binutils215/type-vector-original-inst-order-oct15s/`
and `/tmp/vuc-binutils215/type-vector-real-ctor-oct15s/`.

## Other untouched / previously unselected functions in this batch

The fresh `std::__uninitialized_copy_aux<ScopeIter,ScopeIter>`
at **0x0806a840**, 149 bytes, uses an authentic 513-bucket
`MLHashTable` element of **0x804 bytes** and real
GNU C++03 `std::__uninitialized_copy_aux`. The ordinary template
source `vuc/probes/ScopeNormalIteratorUninitializedFreshOct15s.cpp`
compiles to **136 bytes** with both GCC snapshots. Original ELF
disassembly confirms matching loop/exception semantics, but its
return-path/catch labels include original compiler-generated
alignment not recovered by ordinary candidate source. The bounded
`vuc/probes/evaluate_scope_normal_iterator_human_source_oct15s.py`
checks ten actual STL/placement-new/catch cleanup spellings:
their sizes are 100 or 136 bytes. Label alignment controls produce
153/155/159/191-byte candidates but not original 149.
**The ScopeIter template is not selected, and no code-byte coverage
is claimed for it**.

The fresh `Binder_Bind(ASTModule*)` original at **0x0804e980**,
552 bytes, now has a meaningful period-C++03 source-level draft
`vuc/probes/BinderBindASTModuleFreshOct15s.cpp`. It reproduces
the original scope setup, three `ASTNodeList` bindings at ASTModule
offsets +8/+20/+32, local 100-element `SymbolTable` vector
reservation, `main`-lookup/export flag, and destructor cleanup
paths with 0x804-byte Scope strides. The closest tested compiler
alignment emits **557 bytes**: only five bytes over original.
Substituting the **different** copy-context Scope declaration
with a `throw()`-qualified clear method drastically changes the
catch code to 466 bytes, demonstrating genuine historical class
declaration ownership; that alternate header is not promoted.
The current 557-byte candidate remains **UNRESOLVED**.

The previously extensively probed `TranslateConst` original
at **0x0806bdd0**, 290 bytes, remains blocked at **291
candidate bytes**. The focused
`vuc/probes/evaluate_translate_const_original_branch_hints_oct15s.py`
tests original C++03 Boolean temporary, inverted condition, and
GNU branch-likelihood source hypotheses with both compiler snapshots;
none produces 290. Do not keep repeating semantically identical
Boolean spellings.

The new and previously untouched **668-byte**
`std::vector<std::string>::_M_insert_aux` at `0x0805b040`
was also reconstructed from the actual GNU libstdc++ 3.3 C++03
`std::vector<std::string>` template, with the established original
iterator-copy instantiation in the same translation unit.
`vuc/probes/StringVectorInsertAuxRealOrderOct15s.cpp` has the
original ELF function order, while
`vuc/probes/StringVectorInsertAuxReversedOct15s.cpp`
controls the reversed template-instantiation order. Both recover
the original string construction and assignment library calls.
Neither is an original-length candidate: Jan25 and Dec06 produce
**700 bytes** at `-O2 -frename-registers`, **656** with
`-falign-jumps=4`, **676** with jumps=8, and **672** with
the targeted real-label alignment `jumps=4,labels=2`, versus
the **668** original instruction bytes. Reversing the true
template-instantiation order does not affect the resulting size.
The 161-byte `std::string` iterator-copy remains independently
**161/161 byte-exact with six original relocations** in *both*
same-source-unit versions at normal `-O2 -frename-registers`;
unrelated alignment changes increase its size to 163, so they
cannot be promoted as a true shared compiler configuration.
The string insertion helper remains UNRESOLVED; only genuine
original-size output may be selected.

Finally, the newly strict `AsmOutput_PrintInstruction` remains
**334/382**, all 25 relocations and 154 authentic combined-unit
literal bytes. The focused
`vuc/probes/evaluate_asm_output_printer_stream_sequence_oct15s.py`
checks independent historical ostream call boundaries in that
genuine owner. Most change original length (398 or 414) or introduce
unmapped `std::endl`; the unchanged candidate remains best.

## Inventory and follow-up

This batch increases the 298-function inventory to
**204 EXACT / 36 EXPERIMENTAL / 58 UNRESOLVED**.
The primary next targets are the **44 instruction-byte mismatches
inside the original 525-byte Type insertion**, and the five-byte
original-length residual for `Binder_Bind`, investigated through
real source class/method ownership rather than broad option flags.
All accepted matches must remain intact, and the full original
linked ELF has not been reconstructed byte-identically.

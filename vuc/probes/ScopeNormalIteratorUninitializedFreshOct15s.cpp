// Historical STL normal_iterator copy of MLHashTable vector in Translate TU.
// Original VUC ELF at 0x0806a840, length 149, not previously reconstructed.
#include <vector>
#include <new>
#include <bits/stl_uninitialized.h>
#include "../src/MLHashFunGameString.h"
#include "../src/ScopeCopyTable.h"
struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *,
                    MLHashFun<MLGameString>, 513> Scope;
typedef std::vector<Scope> ScopeVector;
typedef __gnu_cxx::__normal_iterator<Scope *, ScopeVector> ScopeIter;

// Explicit period-C++03 STL instantiation, not handwritten surrogate bytes.
template ScopeIter std::__uninitialized_copy_aux<ScopeIter, ScopeIter>(
    ScopeIter first, ScopeIter last, ScopeIter result, __false_type);

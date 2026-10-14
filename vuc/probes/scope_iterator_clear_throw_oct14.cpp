// Scope iterator-copy hypothesis using the independently exact clear() throw() declaration.
// Remains a forensic probe unless original 149-byte function and relocations match.
#include <vector>
#include <bits/stl_uninitialized.h>
#include "../src/MLHashFunGameString.h"
#include "../src/ScopeCopyTable.h"

struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *,
                    MLHashFun<MLGameString>, 513> SymbolScope;
typedef std::vector<SymbolScope> ScopeVector;
typedef ScopeVector::iterator ScopeIter;

template ScopeIter std::__uninitialized_copy_aux<ScopeIter, ScopeIter>(
    ScopeIter, ScopeIter, ScopeIter, __false_type);

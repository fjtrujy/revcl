// Byte-exact historical GCC 3.3 std::vector<SymbolScope> allocation/copy template.
#include <vector>
#include "MLHashFunGameString.h"
#include "MLHashTable.h"
struct SymbolTableEntry;
typedef MLHashTable<MLGameString,SymbolTableEntry*,
                    MLHashFun<MLGameString>,513> Scope;
typedef std::vector<Scope> ScopeVector;

template Scope *ScopeVector::_M_allocate_and_copy<Scope *>(
    unsigned, Scope *, Scope *);

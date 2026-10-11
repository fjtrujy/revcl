// Byte-exact recovered VUC Binder translation-unit lifecycle functions.
// Exact checks are per-function and use separately recorded compiler flags;
// the historically used TU-wide switches remain unauthenticated.
#include <iostream>
#include <vector>
#include "MLGameString.h"
#include "MLHashFunGameString.h"
#include "MLHashTable.h"

struct SymbolTableEntry;
typedef MLHashTable<MLGameString,SymbolTableEntry*,MLHashFun<MLGameString>,513> Scope;

struct Float4SymbolVector : std::vector<Scope> {
    Float4SymbolVector() { reserve(100); }
};

bool exportModule;
MLGameString enclosingFunID("@@enclosing_function",true);
MLGameString enclosingLoopID("@@enclosing_loop",true);
Float4SymbolVector float4MemSymbols;

// Historical GNU STL vector construction of 0x804-byte symbol scopes.
#include <vector>
#include <bits/stl_uninitialized.h>
#include "../src/MLHashFunGameString.h"
#include "../src/MLHashTable.h"
struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *, MLHashFun<MLGameString>, 513> SymbolScope;

template SymbolScope *std::__uninitialized_copy_aux<SymbolScope *, SymbolScope *>(
    SymbolScope *, SymbolScope *, SymbolScope *, __false_type);

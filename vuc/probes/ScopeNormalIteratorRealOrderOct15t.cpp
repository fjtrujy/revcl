// Recover original SymbolTable.cpp owner order from genuine exact BeginScope.
// Original ELF: Scope vector insertion at 0x806a550, then normal iterator
// uninitialized_copy_aux at 0x806a840; check original 746/149 sizes together.
#include "../src/SymbolTableBeginScope.cpp"
#include <bits/stl_uninitialized.h>
typedef std::vector<Scope> ScopeVector;
typedef ScopeVector::iterator ScopeIter;
template ScopeIter std::__uninitialized_copy_aux<ScopeIter, ScopeIter>(
    ScopeIter, ScopeIter, ScopeIter, __false_type);

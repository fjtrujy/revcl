// Historical SymbolTable.cpp predecessor context for 149-byte Scope copy.
// Instantiates actual normal iterator after genuine Enter/Find/BeginScope/
// EndScope, vector insert and FindCopy in the reconstructed original owner.
#include "../src/SymbolTableUnit.cpp"
#include <bits/stl_uninitialized.h>
typedef std::vector<SymbolScope> ScopeVector;
typedef ScopeVector::iterator ScopeIter;
template ScopeIter std::__uninitialized_copy_aux<ScopeIter, ScopeIter>(
    ScopeIter, ScopeIter, ScopeIter, __false_type);

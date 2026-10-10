// Byte-exact VUC scope stack insertion. The original non-inlined
// MLHashTable copy constructor is declared in the shared type.
#include "MLHashTable.h"
#include "MLHashFunGameString.h"
#include <vector>
struct SymbolTableEntry;
typedef MLHashTable<MLGameString,SymbolTableEntry*,MLHashFun<MLGameString>,513> Scope;

class SymbolTable {
public:
    void BeginScope();
private:
    std::vector<Scope> scopes_;
};

void SymbolTable::BeginScope()
{
    Scope scope;
    scopes_.push_back(scope);
}

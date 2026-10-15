// Byte-exact VUC SymbolTable::EndScope() using the actual scope vector.
// The ordinary std::vector::pop_back spelling has been verified alongside
// original-order Enter, Find and BeginScope methods under GCC 3.3.3.
#include "MLGameString.h"
#include "MLHashTable.h"
#include "MLHashFunGameString.h"
#include <vector>

struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *,
                    MLHashFun<MLGameString>, 513> SymbolScope;
class SymbolTable {
public:
    void EndScope();
private:
    std::vector<SymbolScope> scopes_;
};

void SymbolTable::EndScope()
{
    scopes_.pop_back();
}

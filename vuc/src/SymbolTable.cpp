// Byte-exact VUC SymbolTable::EndScope() provisional source fragment.
// The remaining SymbolTable methods and original class header are unresolved.
#include "MLGameString.h"
#include "MLHashTable.h"

struct SymbolTableEntry;
class SymbolTable {
public:
    void EndScope();
private:
    unsigned reserved_;
    MLHashTable<MLGameString, SymbolTableEntry *,
                MLHashFun<MLGameString>, 513> *scope_;
};

void SymbolTable::EndScope()
{
    --scope_;
    scope_->clear();
}

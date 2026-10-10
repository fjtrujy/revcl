// Byte-exact scoped symbol table lookup using the GCC 3.3 std::vector layout.
#include "MLHashFunGameString.h"
#include "MLHashTable.h"
#include <vector>
struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *, MLHashFun<MLGameString>, 513> SymbolScope;
class SymbolTable {
public:
    SymbolTableEntry *Find(const MLGameString &name, bool localOnly);
private:
    std::vector<SymbolScope> scopes_;
};
SymbolTableEntry *SymbolTable::Find(const MLGameString &name, bool localOnly)
{
    for (int i = scopes_.size() - 1; i >= 0; --i) {
        SymbolTableEntry *result = scopes_[i].FindCopy(name);
        if (result) return result;
        if (localOnly) break;
    }
    return 0;
}

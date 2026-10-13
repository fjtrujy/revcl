// Reconstruct the original SymbolTable::Enter scope-vector and hash-node insertion.
#include "../src/MLGameString.h"
#include "../src/MLHashTable.h"
#include "../src/MLHashFunGameString.h"
#include <vector>
struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *, MLHashFun<MLGameString>, 513> SymbolScope;
struct SymbolNode {
    MLGameString key;
    unsigned hash;
    SymbolTableEntry *value;
    SymbolNode *next;
    SymbolNode(const MLGameString &k, unsigned h, SymbolTableEntry *v) :
        key(k), hash(h), value(v), next(0) {}
};
class SymbolTable {
public:
    void Enter(const MLGameString &name, SymbolTableEntry *entry);
private:
    std::vector<SymbolScope> scopes_;
};
void SymbolTable::Enter(const MLGameString &name, SymbolTableEntry *entry)
{
    if (scopes_.empty()) return;
    SymbolScope &scope = scopes_[scopes_.size() - 1];
    MLHashFun<MLGameString> hasher;
    SymbolNode *node = new SymbolNode(name, hasher.Hash(name), entry);
    unsigned bucket = node->hash % 513;
    node->next = (SymbolNode *)scope.heads[bucket];
    scope.heads[bucket] = (SymbolScope::Node *)node;
}

// Reconstructed SymbolTable.cpp source-unit context for original GCC 3.3.3.
// Find and BeginScope are exact; Enter is a source-level partial reconstruction.
#include "MLGameString.h"
#include "MLHashTable.h"
#include "MLHashFunGameString.h"
#include <vector>
#include <iostream>

struct SymbolTableEntry;
typedef MLHashTable<MLGameString, SymbolTableEntry *,
                    MLHashFun<MLGameString>, 513> SymbolScope;

// A hash entry stores the original 16-byte key/hash/value/next layout.
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
    SymbolTableEntry *Find(const MLGameString &name, bool localOnly);
    void BeginScope();
    void EndScope();
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

SymbolTableEntry *SymbolTable::Find(const MLGameString &name, bool localOnly)
{
    for (int i = scopes_.size() - 1; i >= 0; --i) {
        SymbolTableEntry *result = scopes_[i].FindCopy(name);
        if (result) return result;
        if (localOnly) break;
    }
    return 0;
}

void SymbolTable::BeginScope()
{
    SymbolScope scope;
    scopes_.push_back(scope);
}

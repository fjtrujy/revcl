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
    SymbolScope::Node *node = new SymbolScope::Node(name, hasher.Hash(name), entry);
    unsigned bucket = node->hash % 513;
    node->next = scope.heads[bucket];
    scope.heads[bucket] = node;
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

void SymbolTable::EndScope()
{
    scopes_.pop_back();
}

// The original SymbolTable.cpp weakly instantiates its referenced FindCopy
// after the Scope vector helpers. Preserve this genuine template ownership.
template <class Key, class Value, class Hasher, int Buckets>
Value MLHashTable<Key, Value, Hasher, Buckets>::FindCopy(const Key &key)
{
    unsigned hash = Hasher().Hash(key);
    Node *node = heads[hash % Buckets];
    while (node) {
        if (node->hash == hash && Hasher().IsEqual(node->key, key))
            return node->value;
        node = node->next;
    }
    return Value();
}

template SymbolTableEntry *
MLHashTable<MLGameString, SymbolTableEntry *, MLHashFun<MLGameString>, 513>::
    FindCopy(const MLGameString &key);

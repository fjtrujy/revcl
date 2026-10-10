// Byte-exact FindCopy specialization, kept in a provisional separate unit.
// The historical original unit boundary remains to be reconstructed.
#include "MLGameString.h"
#include "MLHashFunGameString.h"
#include "MLHashTable.h"

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

template MLGameStringData *
MLHashTable<const char *, MLGameStringData *, MLHashFun<const char *>, 513>::
    FindCopy(const char *const &key);

struct SymbolTableEntry;
template SymbolTableEntry *
MLHashTable<MLGameString, SymbolTableEntry *, MLHashFun<MLGameString>, 513>::
    FindCopy(const MLGameString &key);

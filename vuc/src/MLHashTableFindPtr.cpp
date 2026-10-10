// Byte-exact MLHashTable<MLGameString,MLGameString>::FindPtr().
#include "MLHashFunGameString.h"
#include "MLHashTable.h"

template <class Key, class Value, class Hasher, int Buckets>
Value *MLHashTable<Key, Value, Hasher, Buckets>::FindPtr(const Key &key)
{
    unsigned hash = Hasher().Hash(key);
    Node *node = heads[hash % Buckets];
    while (node) {
        if (node->hash == hash && Hasher().IsEqual(node->key, key))
            return &node->value;
        node = node->next;
    }
    return 0;
}
template MLGameString *
MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513>::
    FindPtr(const MLGameString &key);

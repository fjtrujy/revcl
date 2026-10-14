// EXPERIMENTAL VUC MLHashTable<MLGameString,...> copy constructor.
// Original 166-byte body; 80/166 instruction bytes and four authentic calls
// independently verified. This provisional Node constructor is not proven
// to be the actual 2004 source declaration; never accept as byte-exact.
// Provisional original VUC MLHashTable deep-copy constructor.
#include "MLHashTableCopyNodePartial.h"
#include "../src/MLHashFunGameString.h"
#include <new>
struct SymbolTableEntry;
template <class Key, class Value, class Hasher, int Buckets>
MLHashTable<Key,Value,Hasher,Buckets>::MLHashTable(const MLHashTable &other)
{
    for (int i = 0; i < Buckets; ++i) {
        heads[i] = 0;
        Node **tail = &heads[i];
        Node *node = other.heads[i];
        while (node) {
            Node *next = node->next;
            Node *copy = new Node(node->key, node->hash, node->value, node->next);
            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
            node = next;
        }
    }
}
template MLHashTable<MLGameString, SymbolTableEntry *, MLHashFun<MLGameString>, 513>::
MLHashTable(const MLHashTable &other);

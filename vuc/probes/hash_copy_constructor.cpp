// Provisional original VUC MLHashTable deep-copy constructor.
#include "../src/MLHashTable.h"
#include "../src/MLHashFunGameString.h"
#include <new>
struct SymbolTableEntry;
template <class Key, class Value, class Hasher, int Buckets>
MLHashTable<Key,Value,Hasher,Buckets>::MLHashTable(const MLHashTable &other)
{
    for (int i = 0; i < Buckets; ++i) {
        heads[i] = 0;
        Node **tail = &heads[i];
        for (Node *node = other.heads[i]; node; node = node->next) {
            Node *copy = new Node(*node);
            copy->next = 0;
            *tail = copy;
            tail = &copy->next;
        }
    }
}
template MLHashTable<MLGameString, SymbolTableEntry *, MLHashFun<MLGameString>, 513>::
MLHashTable(const MLHashTable &other);

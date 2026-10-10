// Reconstructed VUC MLHashTable.cpp fragment.
// Exact helper bodies are checked independently against the original ELF.
#include "MLGameString.h"
#include <cstring>

template <> class MLHashFun<MLGameString> {
public:
    unsigned Hash(const MLGameString &key);
};

unsigned MLHashFun<MLGameString>::Hash(const MLGameString &key)
{
    return key.data_ ? key.data_->hash : 0;
}

bool MLHashFun<const char *>::IsEqual(const char * const &lhs,
                                      const char * const &rhs)
{
    return strcmp(lhs, rhs) == 0;
}

template <class Key, class Value, class Hasher, int Buckets>
class MLHashTable {
public:
    struct Node {
        Key key;
        Value value;
        unsigned hash;
        Node *next;
    };
    Node *heads[Buckets];
    void clear();
};

template <class Key, class Value, class Hasher, int Buckets>
void MLHashTable<Key, Value, Hasher, Buckets>::clear()
{
    for (int index = 0; index < Buckets; ++index) {
        while (heads[index]) {
            Node *node = heads[index];
            heads[index] = node->next;
            ::operator delete(node);
        }
    }
}

template class MLHashTable<int, int, MLHashFun<int>, 513>;
template class MLHashTable<const char *, MLGameStringData *,
                           MLHashFun<const char *>, 513>;

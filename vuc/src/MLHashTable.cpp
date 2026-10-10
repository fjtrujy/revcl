// Reconstructed VUC MLHashTable.cpp fragment.
// Exact helper bodies are checked independently against the original ELF.
#include "MLGameString.h"
#include "MLHashTable.h"
#include <cstring>

template <> class MLHashFun<MLGameString> {
public:
    unsigned Hash(const MLGameString &key);
    bool IsEqual(const MLGameString &lhs, const MLGameString &rhs);
};

unsigned MLHashFun<MLGameString>::Hash(const MLGameString &key)
{
    return key.data_ ? key.data_->hash : 0;
}

bool MLHashFun<MLGameString>::IsEqual(const MLGameString &lhs,
                                    const MLGameString &rhs)
{
    return lhs == rhs;
}

unsigned MLHashFun<const char *>::Hash(const char *const &value)
{
    unsigned hash = 0;
    const char *first = value;
    const char *last = first + strlen(first);
    while (first < last) {
        for (int n = 2; n >= 0; --n) {
            unsigned char bit = hash & 1;
            hash >>= 1;
            if (bit)
                hash |= 0x80000000u;
        }
        hash += (signed char)*first;
        ++first;
    }
    return hash;
}

bool MLHashFun<const char *>::IsEqual(const char * const &lhs,
                                      const char * const &rhs)
{
    return strcmp(lhs, rhs) == 0;
}

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

template void MLHashTable<int, int, MLHashFun<int>, 513>::clear();
struct SymbolTableEntry;
template void MLHashTable<MLGameString, SymbolTableEntry *,
                          MLHashFun<MLGameString>, 513>::clear();
template class MLHashTable<const char *, MLGameStringData *,
                           MLHashFun<const char *>, 513>;

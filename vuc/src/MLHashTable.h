// Provisional VUC MLHashTable layout from byte-exact node operations.
#ifndef VUC_ML_HASH_TABLE_H
#define VUC_ML_HASH_TABLE_H

template <class Key, class Value, class Hasher, int Buckets>
class MLHashTable {
public:
    struct Node {
        Key key;
        unsigned hash;
        Value value;
        Node *next;
    };
    Node *heads[Buckets];
    MLHashTable() {
        for (int i = 0; i < Buckets; ++i)
            heads[i] = 0;
    }
    // The original non-inlined copy constructor is named in the VUC ELF.
    MLHashTable(const MLHashTable &other);
    ~MLHashTable() { clear(); }
    void clear();
    Value FindCopy(const Key &key);
    Value *FindPtr(const Key &key);
};
#endif

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
    void clear();
    Value FindCopy(const Key &key);
};
#endif

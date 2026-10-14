// Original-shape 513-bucket table declaration for VUC's scope-copy template.
// The historical GCC byte match requires clear() to be declared throw().
// This source-unit view is deliberately separate from provisional declarations
// used by already exact functions; original common-header ownership is unproven.
#ifndef VUC_SCOPE_COPY_TABLE_H
#define VUC_SCOPE_COPY_TABLE_H

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
    MLHashTable(const MLHashTable &other);
    ~MLHashTable() { clear(); }
    void clear() throw();
    Value FindCopy(const Key &key);
    Value *FindPtr(const Key &key);
};
#endif

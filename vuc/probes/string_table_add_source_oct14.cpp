// VUC MLGameStringTable::Add() with ordinary inlined constructors and
// exception unwinding. The exact original 513-bucket structure is preserved.
#include <cstring>
#include <new>
template<class T> struct MLHashFun;
template<> struct MLHashFun<const char *> {
    unsigned Hash(const char * const &value);
};
struct MLGameStringData {
    unsigned hash;
    const char *text;
    unsigned length;
    bool temporary;
    MLGameStringData(const char *input, bool temp) {
        temporary = temp;
        const char *value = input;
        length = strlen(value);
        hash = MLHashFun<const char *>().Hash(value);
        if (temp) text = value;
        else {
            char *copy = new char[length + 1];
            strcpy(copy,value);
            text = copy;
        }
    }
};
template<class K,class V,class H,int N> struct MLHashTable {
    struct Node {
        K key;
        unsigned hash;
        V value;
        Node *next;
        Node(const K &k, const V &v) {
            hash = H().Hash(k);
            key = k;
            value = v;
        }
    };
    Node *heads[N];
    V FindCopy(const K &key);
};
struct MLGameStringTable :
    MLHashTable<const char *, MLGameStringData *,
                MLHashFun<const char *>, 513> {
    MLGameStringData *Add(const char *text, bool temporary);
};
MLGameStringData *MLGameStringTable::Add(const char *text, bool temporary) {
    MLGameStringData *found = FindCopy(text);
    if (found) return found;
    MLGameStringData *data = new MLGameStringData(text, temporary);
    typedef MLHashTable<const char *, MLGameStringData *,
                        MLHashFun<const char *>, 513> Table;
    Table::Node *node = new Table::Node(data->text, data);
    unsigned index = node->hash % 513;
    node->next = heads[index];
    heads[index] = node;
    return data;
}

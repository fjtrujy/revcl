// Source-level reconstruction of VUC 2004.1 MLGameStringTable::Add.
// Original ELF proves 16-byte interned data, 16-byte hash nodes, and
// hash calculation during node construction (after operator new).
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
            strcpy(copy, value);
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
        Node(const K &k, const V &v)
          : key(k), hash(H().Hash(k)), value(v), next(0) {}
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
    MLGameStringData *result = FindCopy(text);
    if (!result) {
        result = new MLGameStringData(text, temporary);
        typedef MLHashTable<const char *, MLGameStringData *,
                            MLHashFun<const char *>, 513> Table;
        Table::Node *node = new Table::Node(result->text, result);
        unsigned index = node->hash % 513;
        node->next = heads[index];
        heads[index] = node;
    }
    return result;
}

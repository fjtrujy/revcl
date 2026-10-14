// Candidate for original MLGameStringTable::Add() (263 bytes).
// This is a source-level reconstruction of the 2004 interning lookup,
// data construction, 513-bucket chaining and temporary string ownership.
#include <cstring>
#include <new>

struct MLGameStringData {
    unsigned hash;
    const char *text;
    unsigned length;
    bool temporary;
    MLGameStringData(const char *value, bool is_temporary) {
        temporary = is_temporary;
        length = strlen(value);
        hash = 0; // set by the source-level wrapper after copy length
        text = value;
    }
};

template<class T> struct MLHashFun;
template<> struct MLHashFun<const char *> {
    unsigned Hash(const char * const &value);
};
template<class K, class V, class H, int N> struct MLHashTable {
    struct Node {
        K key;
        unsigned hash;
        V value;
        Node *next;
        Node(const K &key_arg, const V &value_arg, unsigned h)
          : key(key_arg), hash(h), value(value_arg), next(0) {}
    };
    Node *heads[N];
    V FindCopy(const K &key);
};

struct MLGameStringTable : MLHashTable<const char *, MLGameStringData *, MLHashFun<const char *>,513> {
    MLGameStringData *Add(const char *text, bool temporary);
};

MLGameStringData *MLGameStringTable::Add(const char *text, bool temporary)
{
    MLGameStringData *data = FindCopy(text);
    if (data)
        return data;
    data = new MLGameStringData(text, temporary);
    data->hash = MLHashFun<const char *>().Hash(text);
    if (!temporary) {
        char *copy = new char[data->length+1];
        strcpy(copy,text);
        data->text = copy;
    }
    typedef MLHashTable<const char *, MLGameStringData *, MLHashFun<const char *>, 513> Table;
    unsigned hash = MLHashFun<const char *>().Hash(data->text);
    Table::Node *node = new Table::Node(data->text, data, hash);
    unsigned slot = hash%513;
    node->next = heads[slot];
    heads[slot] = node;
    return data;
}

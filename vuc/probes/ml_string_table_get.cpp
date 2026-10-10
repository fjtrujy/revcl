// VUC MLGameStringTable::Get candidate using base/derived init ordering.
// Experimental until every original relocation and byte matches.
struct MLGameStringData;
template <class T> class MLHashFun;

template <class Key, class Value, class Hasher, int Buckets>
class MLHashTable {
public:
    struct Node {
        Key key;
        Value value;
        unsigned hash;
        Node *next;
    };
    MLHashTable() {
        for (int i = 0; i < Buckets; ++i)
            buckets_[i] = 0;
    }
    ~MLHashTable() { clear(); }
    void clear();
protected:
    Node *buckets_[Buckets];
};

class MLGameStringTable :
    public MLHashTable<const char *, MLGameStringData *,
                       MLHashFun<const char *>, 513> {
public:
    static MLGameStringTable *Get();
    MLGameStringTable() : count_(0), used_(0) {}
private:
    unsigned count_;
    unsigned used_;
};

MLGameStringTable *MLGameStringTable::Get()
{
    static MLGameStringTable table;
    return &table;
}

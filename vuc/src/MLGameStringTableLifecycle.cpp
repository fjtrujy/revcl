// Original-style local static MLGameStringTable singleton.
// Only its compiler-generated __tcf_0 teardown is accepted as byte exact.
// The emitted MLGameStringTable::Get() remains non-exact and unaccepted.
struct MLGameStringData;
template <class T> class MLHashFun;

template <class Key, class Value, class Hasher, int Buckets>
class MLHashTable {
public:
    struct Node {
        Key key;
        unsigned hash;
        Value value;
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
    static MLGameStringTable gst;
    return &gst;
}

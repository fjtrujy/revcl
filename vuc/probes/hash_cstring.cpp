// Experimental reconstruction of historical VUC string hash loop.
#include <cstring>
template <class T> class MLHashFun;

template <> class MLHashFun<const char *> {
public:
    unsigned Hash(const char *const &s);
};

unsigned MLHashFun<const char *>::Hash(const char *const &value)
{
    const char *first = value;
    const char *last = first + strlen(first);
    unsigned hash = 0;
    while (first < last) {
        for (int n = 2; n >= 0; --n) {
            unsigned bit = hash & 1;
            hash >>= 1;
            if (bit)
                hash |= 0x80000000u;
        }
        hash += (signed char)*first;
        ++first;
    }
    return hash;
}

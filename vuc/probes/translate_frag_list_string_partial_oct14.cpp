// Original-length experimental VUC TranslateFragList<MLGameString>.
// 75/306 original instruction bytes with six authentic relocations under
// GCC 3.3.3 (2004-01-25) -O2 -frename-registers -falign-jumps=1.
// The branch hint is a compiler hypothesis, not authenticated author intent.
// Incomplete control-flow/register matching: this must never enter vuc/src.
#include "../src/MLGameString.h"

namespace IR {
struct Data {
    int kind;
    Data() : kind(0) {}
};
struct Fragment_Data : Data {
    virtual ~Fragment_Data();
    MLGameString name;
    explicit Fragment_Data(const MLGameString &value) : Data(), name(value) {}
};
}

template <typename T> struct ASTNodeList {
    int length;
    int capacity;
    T *entries;
    ASTNodeList() : length(0), capacity(0), entries(0) {}
    void Add(T element) {
        int old_size = length;
        if (old_size >= capacity) {
            int next_capacity = capacity ? capacity * 2 : 16;
            T *old_entries = entries;
            entries = new T[next_capacity];
            if (old_entries)
                __builtin_memcpy(entries, old_entries, length * sizeof(T));
            capacity = next_capacity;
        }
        entries[old_size] = element;
        length = old_size + 1;
    }
};

template <typename T>
ASTNodeList<IR::Fragment_Data *> TranslateFragList(ASTNodeList<T> &input)
{
    ASTNodeList<IR::Fragment_Data *> output;
    int i = 0;
    while (__builtin_expect((i < input.length), 1)) {
        int index = i;
        const MLGameString &name = input.entries[index];
        IR::Fragment_Data *fragment = new IR::Fragment_Data(name);
        ++i;
        output.Add(fragment);
    }
    return output;
}

template ASTNodeList<IR::Fragment_Data *>
TranslateFragList<MLGameString>(ASTNodeList<MLGameString> &);

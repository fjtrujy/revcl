// Experimental source-level reconstruction for the MLGameString list case.
// Keep outside vuc/src until exact original length, bytes and relocations pass.
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
    while (i < input.length) {
        int index = i;
        ++i;
        IR::Fragment_Data *fragment = new IR::Fragment_Data(input.entries[index]);
        output.Add(fragment);
    }
    return output;
}

template ASTNodeList<IR::Fragment_Data *>
TranslateFragList<MLGameString>(ASTNodeList<MLGameString> &);

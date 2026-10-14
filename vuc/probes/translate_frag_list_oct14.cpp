// Original VUC TranslateFragList template hypothesis, not accepted source.
// ASTNodeList has three 32-bit members in the original executable.
struct ASTDecl {};
struct ASTVarDecl : ASTDecl {};
namespace IR { struct Fragment_Data; }
IR::Fragment_Data *TranslateFrag(ASTDecl *);

template<class T> struct ASTNodeList {
    int length;
    int capacity;
    T *entries;
    ASTNodeList() : length(0), capacity(0), entries(0) {}
    void Add(T element) {
        if (length >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *new_entries = new T[new_capacity];
            if (entries)
                __builtin_memcpy(new_entries, entries, length * sizeof(T));
            capacity = new_capacity;
            entries = new_entries;
        }
        entries[length++] = element;
    }
};

template<class T>
ASTNodeList<IR::Fragment_Data *> TranslateFragList(ASTNodeList<T> &input)
{
    ASTNodeList<IR::Fragment_Data *> result;
    for (int i = 0; i < input.length; ++i)
        result.Add(TranslateFrag(input.entries[i]));
    return result;
}

template ASTNodeList<IR::Fragment_Data *> TranslateFragList<ASTVarDecl *>(ASTNodeList<ASTVarDecl *> &);
template ASTNodeList<IR::Fragment_Data *> TranslateFragList<ASTDecl *>(ASTNodeList<ASTDecl *> &);

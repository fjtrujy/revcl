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
            T *old_entries = entries;
            entries = new T[new_capacity];
            capacity = new_capacity;
            if (old_entries)
                __builtin_memcpy(entries, old_entries, length * sizeof(T));
        }
        entries[length++] = element;
    }
};

template<class T>
ASTNodeList<IR::Fragment_Data *> TranslateFragList(ASTNodeList<T> &input)
{
    ASTNodeList<IR::Fragment_Data *> result;
    ASTDecl **current = (ASTDecl **)input.entries;
    ASTDecl **end = current + input.length;
    while (current != end) result.Add(TranslateFrag(*current++));
    return result;
}

template ASTNodeList<IR::Fragment_Data *> TranslateFragList<ASTVarDecl *>(ASTNodeList<ASTVarDecl *> &);

// Reconstructed VUC expression-list translator hypothesis; unaccepted.
// The original 256-byte function uses ASTNodeList's three-word layout,
// Translate(ASTExpr *, unsigned, int), and sixteen-element growth.
struct ASTExpr {};
namespace IR { struct Expr_Data; }
IR::Expr_Data *Translate(ASTExpr *, unsigned, int);

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
            if (old_entries)
                __builtin_memcpy(entries, old_entries, length * sizeof(T));
            capacity = new_capacity;
        }
        entries[length++] = element;
    }
};
ASTNodeList<IR::Expr_Data *> Translate(ASTNodeList<ASTExpr *> &input)
{
    ASTNodeList<IR::Expr_Data *> result;
    for (int i = 0; i < input.length; ++i)
        result.Add(Translate(input.entries[i], 0x3c00, 0));
    return result;
}

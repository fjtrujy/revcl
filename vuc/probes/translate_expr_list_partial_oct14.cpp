// Experimental 256-byte VUC ASTExpr list translation candidate.
// The pointer-copy and postincrement source choices reproduce 90/256 bytes.
// Original inline copy is rep movsl, so this is NOT authenticated source.
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
                for (int j=0; j<length; ++j) entries[j] = old_entries[j];
            capacity = new_capacity;
        }
        entries[length++] = element;
    }
};
ASTNodeList<IR::Expr_Data *> Translate(ASTNodeList<ASTExpr *> &input)
{
    ASTNodeList<IR::Expr_Data *> result;
    int i=0;
    while(i<input.length)
        result.Add(Translate(input.entries[i++], 0x3c00, 0));
    return result;
}

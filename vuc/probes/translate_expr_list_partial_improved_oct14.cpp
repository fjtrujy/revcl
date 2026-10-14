// Improved experimental VUC ASTExpr list candidate: 112/256 original bytes.
// GCC 3.3.3 (-O2 -frename-registers -falign-jumps=4
// -fno-reorder-blocks); both original call relocations are authenticated.
// The scalar pointer-copy loop differs from the original rep movsl path.
// This is not original source provenance or a byte-exact reconstruction.
struct ASTExpr {};
namespace IR { struct Expr_Data; }
IR::Expr_Data *Translate(ASTExpr *, unsigned, int);

template<class T> struct ASTNodeList {
    int length;
    int capacity;
    T *entries;
    ASTNodeList() : length(0), capacity(0), entries(0) {}
    void Add(T element) {
        int old_size=length;
        if (old_size >= capacity) {
            int new_capacity = capacity ? 2 * capacity : 16;
            T *old_entries = entries;
            entries = new T[new_capacity];
            if (old_entries)
                for (int j=0; j<length; ++j) entries[j] = old_entries[j];
            capacity = new_capacity;
        }
        entries[old_size]=element; length=old_size+1;
    }
};
ASTNodeList<IR::Expr_Data *> Translate(ASTNodeList<ASTExpr *> &input)
{
    ASTNodeList<IR::Expr_Data *> result;
    int i=0;
    while(i<input.length){
      int index=i; ++i;
      result.Add(Translate(input.entries[index],0x3c00,0));
    }
    return result;
}

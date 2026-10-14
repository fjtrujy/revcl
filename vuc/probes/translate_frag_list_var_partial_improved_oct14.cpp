// Experimental TranslateFragList<ASTVarDecl *> source: 110/240 original bytes.
// Compiled with GCC 3.3.3, -O2 -frename-registers -falign-jumps=4
// -fno-reorder-blocks. Both original call relocations are authenticated.
// This scalar array copy differs from the original rep movsl path.
// This is not byte-exact or established original source provenance.
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

template<class T> ASTNodeList<IR::Fragment_Data *> TranslateFragList(ASTNodeList<T> &input)
{
    ASTNodeList<IR::Fragment_Data *> result;
    int i=0;
    while (i<input.length) {
        int index=i; ++i;
        result.Add(TranslateFrag(input.entries[index]));
    }
    return result;
}
template ASTNodeList<IR::Fragment_Data *> TranslateFragList<ASTVarDecl *>(ASTNodeList<ASTVarDecl *> &);

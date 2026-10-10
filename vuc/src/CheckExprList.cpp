// Exact VUC ASTExpr list checker; original ABI returns a 16-byte Type.
#include "ASTNodeList.h"
struct ASTExpr;
struct Type { unsigned a,b,c,d; };
Type Check(ASTExpr *expr);
template<class T> void Check(ASTNodeList<T> &list) {
    for (int i=0; i<list.length; ++i)
        Check(list.entries[i]);
}
template void Check(ASTNodeList<ASTExpr *> &);

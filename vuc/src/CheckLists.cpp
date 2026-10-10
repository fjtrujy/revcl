// Three byte-exact VUC Check(ASTNodeList<T>&) template instantiations.
#include "ASTNodeList.h"

template <class T>
void Check(ASTNodeList<T> &list)
{
    for (int i = 0; i < list.length; ++i)
        Check(list.entries[i]);
}

template void Check(ASTNodeList<ASTVarDecl *> &);
template void Check(ASTNodeList<ASTStmt *> &);
template void Check(ASTNodeList<ASTDecl *> &);

// Reconstructed VUC list translation / binding specializations.
#include "ASTNodeList.h"

class SymbolTable;
void Translate(ASTDecl *value);
void Bind(ASTDecl *value, SymbolTable &symbols);

template <class T>
void Translate(ASTNodeList<T> &list)
{
    for (int i = 0; i < list.length; ++i)
        Translate(list.entries[i]);
}

template <class T>
void Bind(ASTNodeList<T> &list, SymbolTable &symbols)
{
    for (int i = 0; i < list.length; ++i) {
        T element = list.entries[i];
        Bind(element, symbols);
    }
}
template void Translate(ASTNodeList<ASTVarDecl *> &);
template void Bind(ASTNodeList<ASTVarDecl *> &, SymbolTable &);

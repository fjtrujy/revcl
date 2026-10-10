// Byte-exact VUC Bind loop variants for ASTExpr, ASTStmt and ASTDecl.
// They load the list's entries pointer before building outgoing arguments.
#include "ASTNodeList.h"
#include "MLGameString.h"

struct ASTExpr {};
class SymbolTable;
void Bind(ASTExpr *value, SymbolTable &symbols);
void Bind(ASTStmt *value, SymbolTable &symbols);
void Bind(ASTDecl *value, SymbolTable &symbols);
void Bind(MLGameString &value, SymbolTable &symbols);

template <class T>
void Bind(ASTNodeList<T> &list, SymbolTable &symbols)
{
    for (int i = 0; i < list.length; ++i) {
        T *entries = list.entries;
        Bind(entries[i], symbols);
    }
}

template void Bind(ASTNodeList<ASTExpr *> &, SymbolTable &);
template void Bind(ASTNodeList<ASTStmt *> &, SymbolTable &);
template void Bind(ASTNodeList<ASTDecl *> &, SymbolTable &);
template void Bind(ASTNodeList<MLGameString> &, SymbolTable &);

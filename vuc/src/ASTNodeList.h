// Provisional 12-byte VUC ASTNodeList layout reconstructed from x86 code.
// Not an authenticated original header.
#ifndef VUC_AST_NODE_LIST_H
#define VUC_AST_NODE_LIST_H

struct ASTDecl {};
struct ASTVarDecl : ASTDecl {};
struct ASTStmt {};

template <class T> struct ASTNodeList {
    int length;
    int capacity;
    T *entries;
};

void Check(ASTDecl *decl);
void Check(ASTStmt *stmt);
template <class T> void Check(ASTNodeList<T> &list);

#endif

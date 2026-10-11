// Experimental original C++ ABI reconstruction of ASTFunCall checking.
#include "../src/ASTNodeList.h"
#include "../src/MLGameString.h"
#include <vector>
struct Type {
    unsigned a, b, c, d;
    Type() {}
    Type(const Type &other) : a(other.a), b(other.b), c(other.c), d(other.d) {}
};
struct FunctionSignature {
    Type result;
    unsigned reserved[4];
    MLGameString name;
    std::vector<Type> params;
};
struct ASTLValExpr {
    unsigned reserved[8];
    FunctionSignature *signature;
};
struct ASTExpr;
struct ASTFunCall {
    unsigned reserved[2];
    ASTLValExpr *callee;
    ASTNodeList<ASTExpr *> args;
};
Type Check(ASTLValExpr *);
template <class T> void Check(ASTNodeList<T> &);
void CheckActualParamList(const ASTNodeList<ASTExpr *> &,
                          const std::vector<Type> &,
                          const MLGameString &);
Type Check(ASTFunCall *call)
{
    Check(call->callee);
    Check(call->args);
    CheckActualParamList(call->args,
                         call->callee->signature->params,
                         call->callee->signature->name);
    return call->callee->signature->result;
}

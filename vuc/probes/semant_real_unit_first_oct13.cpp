// Provisional real semantic parameter checks in original function order.
#include <iostream>
// Byte-exact C++03 ASTFunCall checking with implicit 16-byte Type copy.
#include "../src/ASTNodeList.h"
#include "../src/MLGameString.h"
#include <vector>
struct Type {
    unsigned a, b, c, d;
    Type() {}
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
Type Check(ASTExpr *);
template <class T> void Check(ASTNodeList<T> &);
void CheckActualParamList(const ASTNodeList<ASTExpr *> &,
                          const std::vector<Type> &,
                          const MLGameString &);

// Source-level semantic check: compare argument count and types.
// The error-reporting policy is provisional, not a reconstruction claim.
void CheckActualParamList(const ASTNodeList<ASTExpr *> &args,
                          const std::vector<Type> &params,
                          const MLGameString &name)
{
    if (args.length != (int)params.size()) {
        std::cerr << "Incorrect argument count: " << name.c_str() << std::endl;
        return;
    }
    for (int i = 0; i < args.length; ++i) {
        Type actual = Check(args.entries[i]);
        if (actual.a != params[i].a || actual.b != params[i].b) {
            std::cerr << "Incorrect argument type: " << name.c_str() << std::endl;
            return;
        }
    }
}

Type Check(ASTFunCall *call)
{
    Check(call->callee);
    Check(call->args);
    CheckActualParamList(call->args,
                         call->callee->signature->params,
                         call->callee->signature->name);
    return call->callee->signature->result;
}

#include "../src/SemantCheck.cpp"

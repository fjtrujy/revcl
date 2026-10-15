// First original-era C++03 reconstruction of previously unresolved
// Canonize_TryReduceExpr<int/unsigned>; keep source hypotheses in probes.
#include "../src/MLHashTable.h"
#include "../src/MLHashFunGameString.h"
#include <cctype>
#include <cstdlib>

namespace IR {
struct Operator_Data { unsigned vt; unsigned code; };
struct Expr_Data {
    unsigned vt;
    unsigned kind;
    Operator_Data *op;
    Expr_Data *lhs;
    Expr_Data *rhs;
};
}
extern MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513> localEnv;
extern MLHashTable<MLGameString, MLGameString, MLHashFun<MLGameString>, 513> globalEnv;
extern "C" long __strtol_internal(const char*, char**, int, int);
template<class T> bool ReduceBinOp(IR::Operator_Data*,const T&,const T&,T&);

template<class T>
bool Canonize_TryReduceExpr(IR::Expr_Data *expr,T &out)
{
    switch (expr->kind) {
    case 5: {
        T left=0,right=0;
        T accepted=0;
        if(Canonize_TryReduceExpr(expr->lhs,left) &&
           Canonize_TryReduceExpr(expr->rhs,right) &&
           ReduceBinOp(expr->op,left,right,out))
            accepted=1;
        return accepted;
    }
    case 6: {
        IR::Expr_Data *child=expr->lhs;
        if(child->kind!=7)
            return Canonize_TryReduceExpr(child,out);
        MLGameString name(*(MLGameString*)&child->op);
        for (;;) {
            MLGameString *value=localEnv.FindPtr(name);
            if (!value) break;
            name=*value;
        }
        for (;;) {
            MLGameString *value=globalEnv.FindPtr(name);
            if (!value) break;
            name=*value;
        }
        if (!std::isdigit(name.c_str()[0])) return false;
        out=(T)__strtol_internal(name.c_str(),0,10,0);
        return true;
    }
    default: return false;
    }
}

template bool Canonize_TryReduceExpr<unsigned>(IR::Expr_Data*,unsigned&);
template bool Canonize_TryReduceExpr<int>(IR::Expr_Data*,int&);

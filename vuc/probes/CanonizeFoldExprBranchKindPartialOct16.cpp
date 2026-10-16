// Original-length Canonize_FoldExpr C++03 hypothesis at 0x080502c0.
// The source follows the original binary-kind-first branch and redundant
// historical-looking kind assignment. Experimental until 410/410.
#include "../src/MLGameString.h"
#include <iostream>
namespace IR {
struct Operator_Data { unsigned vt; unsigned operation; };
struct Expr_Data {
    virtual void typeInfo();
    unsigned kind;
    Expr_Data():kind(0) {}
    explicit Expr_Data(unsigned k):kind(k) {}
};
struct ConstLit_Data : Expr_Data {
    MLGameString name;
    explicit ConstLit_Data(const MLGameString &v):Expr_Data(7),name(v) {kind=7;}
};
struct ConstExpr_Data : Expr_Data {
    Operator_Data *op;
    Expr_Data *expr;
    ConstExpr_Data(Operator_Data *o,Expr_Data *e):Expr_Data(6),op(o),expr(e) {kind=6;}
};
struct BinOp_Data : Expr_Data {
    Operator_Data *op;
    Expr_Data *left;
    Expr_Data *right;
};
}
extern bool error;
bool IsFoldable(IR::Operator_Data *);
MLGameString ConcatenateBinOp(IR::Operator_Data*,const MLGameString&,const MLGameString&);

IR::Expr_Data *Canonize_FoldExpr(IR::Expr_Data *expr)
{
    if(expr->kind != 6) {
        error = true;
        std::cout << "illegal operand in const expr!";
        return expr;
    }
    IR::ConstExpr_Data *wrapper=(IR::ConstExpr_Data*)expr;
    IR::Expr_Data *child=wrapper->expr;
    if (child->kind != 5) {
        if (child->kind != 7) {
            error = true;
            std::cout << "illegal operand in const expr!";
            return expr;
        }
        IR::ConstLit_Data *copy=new IR::ConstLit_Data(((IR::ConstLit_Data*)child)->name);
        return new IR::ConstExpr_Data(wrapper->op,copy);
    }
    IR::BinOp_Data *binary=(IR::BinOp_Data*)child;
    binary->left=Canonize_FoldExpr(binary->left);
    binary->right=Canonize_FoldExpr(binary->right);
    if(!IsFoldable(binary->op) ||
       binary->left->kind!=6 ||
       ((IR::ConstExpr_Data*)binary->left)->expr->kind!=7 ||
       binary->right->kind!=6 ||
       ((IR::ConstExpr_Data*)binary->right)->expr->kind!=7)
        return expr;
    MLGameString text=ConcatenateBinOp(binary->op,
       ((IR::ConstLit_Data*)((IR::ConstExpr_Data*)binary->left)->expr)->name,
       ((IR::ConstLit_Data*)((IR::ConstExpr_Data*)binary->right)->expr)->name);
    IR::ConstLit_Data *copy=new IR::ConstLit_Data(text);
    return new IR::ConstExpr_Data(((IR::ConstExpr_Data*)binary->right)->op,copy);
}

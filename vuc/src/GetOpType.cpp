// Byte-exact VUC operator-expression type inference using provisional IR ABI.
#include <new>
namespace IR {
struct Type_Data {
    virtual void zzzzMakeRTTIWork();
    int kind;
    Type_Data(int k) : kind(k) {}
};
struct UInt_Data : Type_Data { UInt_Data() : Type_Data(1) {} };
struct Operator_Data { void *vtable; int kind; Type_Data *type; };
struct Expr_Data { void *vtable; int kind; Operator_Data *op; };
}
IR::Type_Data *GetOpType(IR::Expr_Data *expr)
{
    if (expr->kind == 5) {
        IR::Operator_Data *op = expr->op;
        if (op->kind == 11) return op->type;
        if (op->kind == 13) return op->type;
        if (op->kind == 12) return op->type;
        if (op->kind == 10) return op->type;
        if (op->kind == 7) return op->type;
        if (op->kind == 6) return op->type;
        if (op->kind == 5) return op->type;
        if (op->kind == 4) return op->type;
        if (op->kind == 3) return op->type;
        if (op->kind == 2) return op->type;
    }
    return new IR::UInt_Data();
}

// Reconstructed VUC Value.cpp fragment.
// The two constructor entries and ASTExpr dispatch are byte-exact.
struct ASTExpr {
    unsigned field0, field1;
    unsigned kind;
};
struct ASTLValExpr;
struct ASTConstValExpr;
struct Value {
    unsigned a, b, c;
    unsigned char d, e, f, g;
    Value();
    void Set(const ASTExpr *expr);
    void Set(const ASTLValExpr *expr);
    void Set(const ASTConstValExpr *expr);
};

Value::Value() : a(0), b(0), c(0), d(0), e(0), f(0), g(0) {}

void Value::Set(const ASTExpr *expr)
{
    if (expr->kind == 2)
        Set((const ASTConstValExpr *)expr);
    else if (expr->kind == 3)
        Set((const ASTLValExpr *)expr);
}

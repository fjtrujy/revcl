#include <iostream>
struct ASTExpr { unsigned field0, field1; unsigned kind; };
struct ASTLValExpr;
struct ASTConstValExpr;
// Reconstructed VUC Value.cpp source-unit context. Constructor and ASTExpr
// dispatch are exact, and ordinary arithmetic/comparison members recover the
// historical global initialization wrapper's register selection.
// Additional member bodies and the complete original source unit remain partial.
struct Value {
    int type; unsigned a, b;
    unsigned char flag0, flag1, flag2, flag3;
    union { int bits; float values[4]; };
    Value();
    static bool TypeConvert(const Value &,const Value &,Value &,Value &);
    Value operator*(const Value &other) const;
    void Set(const ASTExpr *expr);
    void Set(const ASTLValExpr *expr);
    void Set(const ASTConstValExpr *expr);
    Value operator+(const Value &other) const;
    Value operator==(const Value &other) const;
    Value operator!=(const Value &other) const;
    Value operator>(const Value &other) const;
    Value operator<(const Value &other) const;
    Value operator>=(const Value &other) const;
    Value operator<=(const Value &other) const;
};
Value::Value() : type(0),a(0),b(0),flag0(0),flag1(0),flag2(0),flag3(0) {}
void Value::Set(const ASTExpr *expr)
{
    if (expr->kind == 2)
        Set((const ASTConstValExpr *)expr);
    else if (expr->kind == 3)
        Set((const ASTLValExpr *)expr);
}

Value Value::operator+(const Value &other) const
{
    Value result;
    Value temporary;
    if (TypeConvert(*this, other, result, temporary)) {
        switch (result.type) {
        case 1: result.bits += temporary.bits; break;
        case 3: result.values[0] += temporary.values[0]; break;
        case 4:
            result.values[0] += temporary.values[0];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[3] += temporary.values[3];
            break;
        }
    }
    return result;
}

Value Value::operator*(const Value &other) const
{
    Value result;
    Value temporary;
    if (TypeConvert(*this, other, result, temporary)) {
        switch (result.type) {
        case 1: result.bits *= temporary.bits; break;
        case 3: result.values[0] *= temporary.values[0]; break;
        case 4:
            result.values[0] *= temporary.values[0];
            result.values[1] *= temporary.values[1];
            result.values[2] *= temporary.values[2];
            result.values[3] *= temporary.values[3];
            break;
        }
    }
    return result;
}

Value Value::operator==(const Value &other) const {
    Value result;
    Value temporary;
    result.type=2;
    if (TypeConvert(*this,other,result,temporary)) {
        switch (result.type) {
        case 1: case 2: result.bits = (result.bits==temporary.bits);break;
        case 3: result.bits = (result.values[0]==temporary.values[0]);break;
        case 4:
            result.bits=(result.values[0]==temporary.values[0] &&
                         result.values[1]==temporary.values[1] &&
                         result.values[2]==temporary.values[2] &&
                         result.values[3]==temporary.values[3]);
            break;
        }
    }
    return result;
}
Value Value::operator!=(const Value &other) const {
    Value result;
    Value temporary;
    result.type=2;
    if (TypeConvert(*this,other,result,temporary)) {
        switch (result.type) {
        case 1: case 2: result.bits = (result.bits!=temporary.bits);break;
        case 3: result.bits = (result.values[0]!=temporary.values[0]);break;
        case 4:
            result.bits=(result.values[0]!=temporary.values[0] &&
                         result.values[1]!=temporary.values[1] &&
                         result.values[2]!=temporary.values[2] &&
                         result.values[3]!=temporary.values[3]);
            break;
        }
    }
    return result;
}
Value Value::operator>(const Value &other) const {
    Value result;
    Value temporary;
    result.type=2;
    if (TypeConvert(*this,other,result,temporary)) {
        switch (result.type) {
        case 1: case 2: result.bits = (result.bits>temporary.bits);break;
        case 3: result.bits = (result.values[0]>temporary.values[0]);break;
        case 4:
            result.bits=(result.values[0]>temporary.values[0] &&
                         result.values[1]>temporary.values[1] &&
                         result.values[2]>temporary.values[2] &&
                         result.values[3]>temporary.values[3]);
            break;
        }
    }
    return result;
}

Value Value::operator<(const Value &other) const {
    Value result;
    Value temporary;
    result.type=2;
    if (TypeConvert(*this,other,result,temporary)) {
        switch (result.type) {
        case 1: case 2: result.bits = (result.bits<temporary.bits);break;
        case 3: result.bits = (result.values[0]<temporary.values[0]);break;
        case 4:
            result.bits=(result.values[0]<temporary.values[0] &&
                         result.values[1]<temporary.values[1] &&
                         result.values[2]<temporary.values[2] &&
                         result.values[3]<temporary.values[3]);
            break;
        }
    }
    return result;
}

Value Value::operator>=(const Value &other) const {
    Value result;
    Value temporary;
    result.type=2;
    if (TypeConvert(*this,other,result,temporary)) {
        switch (result.type) {
        case 1: case 2: result.bits = (result.bits>=temporary.bits);break;
        case 3: result.bits = (result.values[0]>=temporary.values[0]);break;
        case 4:
            result.bits=(result.values[0]>=temporary.values[0] &&
                         result.values[1]>=temporary.values[1] &&
                         result.values[2]>=temporary.values[2] &&
                         result.values[3]>=temporary.values[3]);
            break;
        }
    }
    return result;
}

Value Value::operator<=(const Value &other) const {
    Value result;
    Value temporary;
    result.type=2;
    if (TypeConvert(*this,other,result,temporary)) {
        switch (result.type) {
        case 1: case 2: result.bits = (result.bits<=temporary.bits);break;
        case 3: result.bits = (result.values[0]<=temporary.values[0]);break;
        case 4:
            result.bits=(result.values[0]<=temporary.values[0] &&
                         result.values[1]<=temporary.values[1] &&
                         result.values[2]<=temporary.values[2] &&
                         result.values[3]<=temporary.values[3]);
            break;
        }
    }
    return result;
}

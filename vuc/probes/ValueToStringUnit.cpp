// Source-context reconstruction of Value::ToString; experimental only.
// Preserves genuine +, -, * methods preceding the original 221-byte routine.
#include <cstdio>
#include "../src/MLGameString.h"
// Recovered VUC Value arithmetic source-unit context.
// Original addition, subtraction and multiplication appear in this order.
// Experimental VUC typed arithmetic using original Value record layout.
struct Value {
    int type;
    unsigned a, b;
    unsigned char flag0, flag1, flag2, flag3;
    union { unsigned bits; float values[4]; };
    Value() : type(0), a(0), b(0), flag0(0), flag1(0), flag2(0), flag3(0) {}
    static bool TypeConvert(const Value&,const Value&,Value&,Value&);
    Value operator+(const Value &other) const;
    Value operator*(const Value &other) const;
    Value operator-(const Value &other) const;
    MLGameString ToString() const;
};
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
Value Value::operator-(const Value &other) const
{
    Value result;
    Value temporary;
    if (TypeConvert(*this, other, result, temporary)) {
        switch (result.type) {
        case 1: result.bits -= temporary.bits; break;
        case 3: result.values[0] -= temporary.values[0]; break;
        case 4:
            result.values[0] -= temporary.values[0];
            result.values[1] -= temporary.values[1];
            result.values[2] -= temporary.values[2];
            result.values[3] -= temporary.values[3];
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

MLGameString Value::ToString() const
{
    char buffer[64] = "";
    switch (type) {
    case 1:
    case 2:
        std::sprintf(buffer, "%d", bits);
        break;
    case 3:
        std::sprintf(buffer, "%f", values[0]);
        break;
    case 4:
        std::sprintf(buffer, "%f,%f,%f,%f",
                     values[0], values[1], values[2], values[3]);
        break;
    }
    return MLGameString(buffer, false);
}

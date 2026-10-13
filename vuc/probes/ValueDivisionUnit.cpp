// Original-order arithmetic predecessor context for experimental division.
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
    Value operator/(const Value &other) const;
    Value operator-(const Value &other) const;
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

Value Value::operator/(const Value &other) const
{
    Value result;
    Value temporary;
    if (TypeConvert(*this, other, result, temporary)) {
        switch (result.type) {
        case 1: result.bits = (int)result.bits / (int)temporary.bits; break;
        case 3: { float divisor = temporary.values[0]; result.values[0] /= divisor; break; }
        case 4:
            result.values[0] *= 1.0f / temporary.values[0];
            result.values[1] *= 1.0f / temporary.values[1];
            result.values[2] *= 1.0f / temporary.values[2];
            result.values[3] *= 1.0f / temporary.values[3];
            break;
        }
    }
    return result;
}

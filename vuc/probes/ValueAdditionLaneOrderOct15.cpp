// EXPERIMENTAL original-length VUC Value::operator+ C++03 source candidate.
// Four independent vector lane additions are ordered 3,1,2,0.
// Historic GCC 3.3.3 and GNU as 2.15 reproduce 203/219 original bytes
// and the authentic TypeConvert relocation; NOT byte-exact acceptance.
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
            result.values[3] += temporary.values[3];
            result.values[1] += temporary.values[1];
            result.values[2] += temporary.values[2];
            result.values[0] += temporary.values[0];
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

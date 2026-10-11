// Byte-exact VUC Value bitwise operations on the original 20-byte ABI layout.
enum TypeID { ValueInteger = 1 };
struct Value {
    int type;
    unsigned a, b;
    unsigned char flag0, flag1, flag2, flag3;
    unsigned bits;
    Value() : type(0), a(0), b(0), flag0(0), flag1(0), flag2(0), flag3(0) {}
    static bool TypeConvertForce(TypeID kind, const Value &first,
                                 const Value &second, Value &result,
                                 Value &temporary);
    Value operator&(const Value &other) const;
    Value operator|(const Value &other) const;
};

Value Value::operator&(const Value &other) const
{
    Value result;
    Value temporary;
    result.type = 1;
    if (TypeConvertForce(ValueInteger, *this, other, result, temporary))
        result.bits &= temporary.bits;
    return result;
}
Value Value::operator|(const Value &other) const
{
    Value result;
    Value temporary;
    result.type = 1;
    if (TypeConvertForce(ValueInteger, *this, other, result, temporary))
        result.bits |= temporary.bits;
    return result;
}

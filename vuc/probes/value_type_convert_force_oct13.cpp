// Experimental ordinary C++ reconstruction of VUC Value::TypeConvertForce.
enum TypeID { ValueUnknown = 0, ValueInteger = 1 };
struct Value {
    int type;
    unsigned a,b;
    unsigned char flag0,flag1,flag2,flag3;
    union { int bits; float values[4]; };
    static bool TypeConvertForce(TypeID desired, const Value &first,
                                 const Value &second, Value &left, Value &right);
};
bool Value::TypeConvertForce(TypeID desired, const Value &first,
                             const Value &second, Value &left, Value &right)
{
    left=first;
    right=second;
    if(first.type == desired && second.type == desired)
        return true;
    if(desired != ValueInteger)
        return false;
    switch(left.type) {
    case 2:
        left.type=2;
        break;
    case 3:
        left.type=3;
        left.bits=(int)left.values[0];
        break;
    default:
        return false;
    }
    switch(right.type) {
    case 2:
        right.type=2;
        break;
    case 3:
        right.type=3;
        right.bits=(int)right.values[0];
        break;
    default:
        return false;
    }
    return true;
}

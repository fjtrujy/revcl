// Experimental genuine TypeConvert predecessor before TypeConvertForce.
// Function-level near-match; the historical compiler is the sole machine-code source.
// Experimental ordinary C++ reconstruction of VUC Value::TypeConvertForce.
enum TypeID { ValueUnknown = 0, ValueInteger = 1 };
struct Value {
    int type;
    unsigned a,b;
    unsigned char flag0,flag1,flag2,flag3;
    union { int bits; float values[4]; };
    static bool TypeConvert(const Value&,const Value&,Value&,Value&);
    static bool TypeConvertForce(TypeID desired, const Value &first,
                                 const Value &second, Value &left, Value &right);
};
bool Value::TypeConvert(const Value &first, const Value &second,
                        Value &left, Value &right)
{
    left = first;
    right = second;
    if (first.type == second.type)
        return true;

    switch (left.type) {
    case 1: case 2:
        switch (right.type) {
        case 1: case 2:
            right.type = 1;
            return true;
        case 3:
            left.values[0] = (float)left.bits;
            left.type = 3;
            return true;
        }
        break;
    case 3:
        switch (right.type) {
        case 1: case 2:
            right.values[0] = (float)right.bits;
            right.type = 3;
            return true;
        }
        break;
    }
    return false;
}

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
    case 3:
        left.type=3;
        left.bits=(int)left.values[0];
        break;
    case 2: left.type=2; break;
    default:
        return false;
    }
    switch(right.type) {
    case 3:
        right.type=3;
        right.bits=(int)right.values[0];
        break;
    case 2: right.type=2; break;
    default:
        return false;
    }
    return true;
}

// Experimental VUC Value::TypeConvert reconstruction from original ELF CFG.
// All fields and conversions are ordinary C++03; no machine-code forcing.
struct Value {
    int type;
    unsigned a, b;
    unsigned char flag0, flag1, flag2, flag3;
    union { int bits; float values[4]; };
    static bool TypeConvert(const Value &first, const Value &second,
                            Value &left, Value &right);
};

bool Value::TypeConvert(const Value &first, const Value &second,
                        Value &left, Value &right)
{
    left = first;
    right = second;
    if (first.type == second.type)
        return true;

    if (left.type == 1 || left.type == 2) {
        if (right.type == 1 || right.type == 2) {
            right.type = 1;
            return true;
        }
        if (right.type == 3) {
            left.values[0] = (float)left.bits;
            left.type = 3;
            return true;
        }
        return false;
    }
    if (left.type == 3 && (right.type == 1 || right.type == 2)) {
        right.values[0] = (float)right.bits;
        right.type = 3;
        return true;
    }
    return false;
}

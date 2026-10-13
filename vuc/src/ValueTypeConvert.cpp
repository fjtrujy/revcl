// Recovered VUC Value::TypeConvert with authentic 32-byte Value copies.
// GCC 3.3.3 emits the original nested-dispatch and shared false return.
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

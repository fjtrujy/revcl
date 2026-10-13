// Experimental original-era VUC Value::ToString implementation.
// Recovered from the original 221-byte routine and its authenticated formats.
#include "../src/MLGameString.h"
#include <cstdio>
struct Value {
    int type;
    unsigned a, b;
    unsigned char flag0, flag1, flag2, flag3;
    union { int bits; float values[4]; };
    MLGameString ToString() const;
};
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

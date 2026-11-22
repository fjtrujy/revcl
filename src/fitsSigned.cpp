#include "Instr.h"
#include "xpNode.h"
#include "xpTree_inline.h"
#include "os.h"

const char *immediateValueTypeName(int type);
extern bool parseFloatImmediateMode;


int fitsSigned(int value, int bits)
{
    int r = 0;
    int hi = 1 << (bits - 1);
    int lo = -hi;
    if (value >= lo && value < hi)
        r = 1;
    return r;
}

bool fitsUnsigned(int value, int bits)
{
    return value >= 0 && value < (1 << bits);
}

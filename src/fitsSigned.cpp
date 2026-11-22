#include "Instr.h"
#include "xpNode.h"
#include "xpTree_inline.h"
#include "os.h"

const char *immediateValueTypeName(int type);
extern bool parseFloatImmediateMode;


int fitsSigned(int value, int bits)
{
    int fitsRange = 0;
    int signedUpperBound = 1 << (bits - 1);
    int signedLowerBound = -signedUpperBound;
    if (value >= signedLowerBound && value < signedUpperBound)
        fitsRange = 1;
    return fitsRange;
}

bool fitsUnsigned(int value, int bits)
{
    return value >= 0 && value < (1 << bits);
}

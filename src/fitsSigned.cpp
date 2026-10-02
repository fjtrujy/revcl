#include "Instr.h"
#include "xpNode.h"
#include "xpTree_inline.h"
#include "os.h"

const char *func_0804ca58(int type);
extern bool g_080b3088;


int fitsSigned(int v, int bits)
{
    int r = 0;
    int hi = 1 << (bits - 1);
    int lo = -hi;
    if (v >= lo && v < hi)
        r = 1;
    return r;
}

bool fitsUnsigned(int v, int bits)
{
    return v >= 0 && v < (1 << bits);
}

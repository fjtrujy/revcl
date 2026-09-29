#pragma implementation "prune.h"
#include "prune.h"

bool Prune(Program *p)
{
    if (!initPrune(p)) {
        func_0804ad38("failed to init prune!\n");
        return false;
    }
    int n;
    do {
        n = tryPrune(p);
        if (n < 0) {
            func_0804ad38("flowMon::Prune - an error occured\n");
            func_0804adf0(-1);
        }
    } while (n);
    if (!doPrune(p)) {
        func_0804ad38("failed to do prune!\n");
        return false;
    }
    return true;
}

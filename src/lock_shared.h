#ifndef LOCK_SHARED_H
#define LOCK_SHARED_H

#include "Instr.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"
#include "Block.h"
#include "statCounts.h"

extern int errorOccurred;
extern bool quietDiagnostics;

static inline bool isRegLock(cls0805bcc4 *lk)
{
    return lk->ownerType == 2;
}

static inline void wrapRange(int &s, int &e)
{
    if (activeLoopSize) {
        while (s >= activeLoopSize) {
            s -= activeLoopSize;
            e -= activeLoopSize;
        }
    }
}

static inline void wrapRangeLocal(int &s, int &e)
{
    int n = activeLoopSize;
    if (n) {
        while (s >= n) {
            s -= n;
            e -= n;
        }
    }
}

#endif

#ifndef LOCK_SHARED_H
#define LOCK_SHARED_H

#include "Instr.h"
#include "os.h"
#include "instStream.h"
#include "workTable.h"
#include "Block.h"
#include "statCounts.h"

extern int errorOccurred;
extern bool quiet_080bbd98;

static inline bool isRegLock(cls0805bcc4 *lk)
{
    return lk->ownerType == 2;
}

static inline void wrapRange(int &s, int &e)
{
    if (g_080b336c) {
        while (s >= g_080b336c) {
            s -= g_080b336c;
            e -= g_080b336c;
        }
    }
}

static inline void wrapRangeLocal(int &s, int &e)
{
    int n = g_080b336c;
    if (n) {
        while (s >= n) {
            s -= n;
            e -= n;
        }
    }
}

#endif

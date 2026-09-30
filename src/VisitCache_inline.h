// VisitCache members (init/release/Extend/startRecord/setBit/endRecord) that only flowMon.cpp inlines; other units (backTrace.cpp) see just
// the declarations in VisitCache.h and call the out-of-line copies.
#ifndef VISITCACHE_INLINE_H
#define VISITCACHE_INLINE_H

#pragma interface

#include <stdlib.h>
#include "VisitCache.h"

inline VisitCache::VisitCache()
{
    unk_14 = false;
    unk_00 = 0;
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
}

inline VisitCache::~VisitCache()
{
    release();
}

inline bool VisitCache::init(int nbits)
{
    release();
    unk_08 = nbits;
    unk_04 = (nbits + 31) / 32;
    if (!Extend(512)) {
        func_0804ad38("VisitCache:: init failed\n");
        release();
        return false;
    }
    return true;
}

inline bool VisitCache::release()
{
    if (unk_00)
        free(unk_00);
    unk_00 = 0;
    unk_04 = 0;
    unk_08 = 0;
    unk_0c = 0;
    unk_10 = 0;
    return true;
}

inline bool VisitCache::Extend(int rows)
{
    unsigned *p = (unsigned *)realloc(unk_00, rows * unk_04 * sizeof(unsigned));
    if (!p) {
        func_0804ad38("VisitCache::Extend.. failed to realloc\n");
        release();
        return false;
    }
    unk_00 = p;
    unk_0c = rows;
    return true;
}

inline bool VisitCache::startRecord()
{
    if (unk_10 >= unk_0c - 1) {
        if (!Extend(unk_0c + 512)) {
            func_0804ad38("startRecord - failed to extend\n");
            return false;
        }
    }
    if (!unk_00) {
        func_0804ad38("VisitCache:: startRecord - null table pointer\n");
        return false;
    }
    unsigned *row = unk_00 + unk_10 * unk_04;
    unk_14 = true;
    for (int i = 0; i != unk_04; i++)
        *row++ = 0;
    return true;
}

inline bool VisitCache::setBit(int bit)
{
    unsigned *row = unk_00 + unk_10 * unk_04;
    row[bit / 32] |= 1 << (bit & 31);
    return true;
}

inline int VisitCache::endRecord()
{
    int n = unk_10;
    unsigned *cur = unk_00 + n * unk_04;
    unsigned *row = unk_00;
    int i;
    for (i = 0; i != n; i++) {
        int j;
        for (j = 0; j != unk_04; j++)
            if (row[j] != cur[j])
                break;
        if (j == unk_04)
            return i;
        row += unk_04;
    }
    unk_10 = n + 1;
    return i;
}

inline bool VisitCache::testBit(int rec, int bit)
{
    unsigned *row = unk_00 + rec * unk_04;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

#endif

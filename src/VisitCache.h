// Growable table of bit rows used by back tracing ("VisitCache"); members are inline,
// emitted via #pragma implementation.
#ifndef VISITCACHE_H
#define VISITCACHE_H

#pragma interface

#include <stdlib.h>
#include "os.h"

class VisitCache {
public:
    unsigned *unk_00;   // table, rows of unk_04 words
    int unk_04;         // words per row
    int unk_08;         // bits per row
    int unk_0c;         // allocated rows
    int unk_10;         // current record
    bool unk_14;        // record started

    VisitCache()
    {
        unk_14 = false;
        unk_00 = 0;
        unk_04 = 0;
        unk_08 = 0;
        unk_0c = 0;
        unk_10 = 0;
    }
    ~VisitCache()
    {
        release();
    }
    bool init(int nbits)
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
    bool release()
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
    bool Extend(int rows)
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
    bool startRecord()
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
};

#endif

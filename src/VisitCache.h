// Growable table of bit rows used by back tracing ("VisitCache"); members are inline,
// emitted via #pragma implementation in flowMon.cpp. init/startRecord/endRecord live in
// VisitCache_inline.h (only flowMon.cpp includes it), so other units call them.
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
    ~VisitCache();
    bool init(int nbits);
    bool release();
    bool Extend(int rows);
    bool startRecord();
    bool setBit(int bit)
    {
        unsigned *row = unk_00 + unk_10 * unk_04;
        row[bit / 32] |= 1 << (bit & 31);
        return true;
    }
    int endRecord();
    bool testBit(int rec, int bit)
    {
        unsigned *row = unk_00 + rec * unk_04;
        return (row[bit / 32] & (1 << (bit & 31))) != 0;
    }
    void Dump()
    {
        func_0804acb0("VisitCache::Dump()\n");
        unsigned *row = unk_00;
        for (int i = 0; i != unk_10; i++) {
            func_0804ac50(" [%3d] ", i);
            for (int j = 0; j != unk_04; j++)
                func_0804ac50(" %08x", row[j]);
            func_0804ac50("\n");
            row += unk_04;
        }
        func_0804acb0("VisitCache:: %d records\n", unk_10);
    }
};

#endif

#ifndef VISITCACHE_H
#define VISITCACHE_H

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

    VisitCache();
    ~VisitCache();
    bool init(int nbits);
    bool release();
    bool Extend(int rows);
    bool startRecord();
    bool setBit(int bit);
    int endRecord();
    bool testBit(int rec, int bit);
    void Dump();
};

#endif

#ifndef VISITCACHE_H
#define VISITCACHE_H

#include <stdlib.h>
#include "os.h"

class VisitCache {
public:
    unsigned *recordData;
    int wordsPerRecord;
    int bitsPerRecord;
    int recordCapacity;
    int recordCount;
    bool recordStarted;

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

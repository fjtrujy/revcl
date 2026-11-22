#ifndef SUBCACHE_H
#define SUBCACHE_H

#include "os.h"

class subCache {
public:
    int *cachedValues;
    unsigned maximumFlowId;
    unsigned wordsPerFlowRow;
    unsigned allocatedWords;

    void init(int fuidMax);
    void release();
    void flush();
    int offset(unsigned fuid);
};

#endif

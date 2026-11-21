#ifndef SUBCACHE_H
#define SUBCACHE_H

#include "os.h"

class subCache {
public:
    int *unk_00;        // table
    unsigned unk_04;    // fuid max
    unsigned unk_08;    // words per row
    unsigned unk_0c;    // total words

    void init(int fuidMax);
    void release();
    void flush();
    int offset(unsigned fuid);
};

#endif

// Per-flow-UID bit table ("subCache"); inline members are emitted by subCache.cpp.
#ifndef SUBCACHE_H
#define SUBCACHE_H

#pragma interface

#include "os.h"

class subCache {
public:
    int *unk_00;        // table
    unsigned unk_04;    // fuid max
    unsigned unk_08;    // words per row
    unsigned unk_0c;    // total words

    void init(int fuidMax)
    {
        release();
        if (fuidMax <= 0)
            func_0804ae34("fuid_max is <=0 in sub cache init\n");
        unk_08 = (unsigned)(fuidMax + 3) >> 2;
        unk_0c = unk_08 * (fuidMax + 1);
        unk_04 = fuidMax;
        unk_00 = new int[unk_0c];
        if (!unk_00)
            func_0804ae34("failed to get table for subc\n");
        flush();
    }
    void release()
    {
        if (unk_00)
            delete[] unk_00;
        unk_00 = 0;
        unk_04 = 0;
        unk_08 = 0;
    }
    void flush()
    {
        if (!unk_00)
            func_0804ae34("subcache flush - no table\n");
        for (unsigned i = 0; i != unk_0c; i++)
            unk_00[i] = 0;
    }
    int offset(unsigned fuid)
    {
        if (!unk_00)
            func_0804ae34("no table set for subCache\n");
        if (fuid > unk_04)
            func_0804ae34("out of bounds fuid for subCache\n");
        return fuid * unk_08 * 4;
    }
};

#endif

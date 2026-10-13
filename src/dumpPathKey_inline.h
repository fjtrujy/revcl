// Back-trace diagnostics emitted after VisitCache members by VisitCache.cpp.
#ifndef DUMP_PATH_KEY_INLINE_H
#define DUMP_PATH_KEY_INLINE_H

#pragma interface

#include "Program.h"
#include "instStream.h"
#include "os.h"
#include "VisitCache.h"

static inline bool testPathBit(VisitCache *vc, int rec, int bit)
{
    unsigned *row = vc->unk_00 + rec * vc->unk_04;
    return (row[bit / 32] & (1 << (bit & 31))) != 0;
}

inline void dumpPathKey(Program *p, int key)
{
    func_0804acb0("dumping path key '%d'\n", key);
    for (instStream *s = p->unk_a0; s; s = s->unk_108) {
        if (testPathBit(&p->unk_bc, key, s->unk_2c0))
            func_0804acb0("    %d: %s\n", s->func_0806d674());
    }
    func_0804acb0(" done dumping path key '%d'\n", key);
}

#endif

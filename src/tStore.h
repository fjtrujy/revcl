// Per-stream "t store" bit sets (Program::unk_b8 words each); inline helpers, emitted via
// #pragma implementation in backTrace.cpp.
#ifndef TSTORE_H
#define TSTORE_H

#pragma interface

#include "Program.h"
#include "os.h"

inline bool zeroTstore(Program *p, unsigned *t)
{
    if (!t) {
        func_0804ad38("zeroTstore - it's null\n");
        return false;
    }
    for (int i = 0; i != p->unk_b8; i++)
        t[i] = 0;
    return true;
}

inline bool initTstore(Program *p, unsigned *t, unsigned bit)
{
    if (!zeroTstore(p, t)) {
        func_0804ad38("failed to init tstore\n");
        return false;
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)p->unk_b8) {
        func_0804ad38("out of bounds t store access\n");
        return false;
    }
    t[idx] = mask;
    return true;
}

inline bool testTstore(Program *p, unsigned *t, unsigned bit)
{
    if (!t) {
        func_0804ad38("store not set for test tstore\n");
        func_0804adf0(-1);
    }
    unsigned idx = bit >> 5;
    unsigned mask = (1 << bit) & 0x1f;
    if (idx >= (unsigned)p->unk_b8) {
        func_0804ad38("out of bounds t store access\n");
        return false;
    }
    return (t[idx] & mask) != 0;
}

#endif

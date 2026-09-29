// Inline Instr dynamic-range helpers, emitted out of line by lock.cpp.
#ifndef INSTRDYN_H
#define INSTRDYN_H
#pragma interface

#include "Instr.h"

inline void Instr::func_0805f4e0()
{
    unk_13f0.unk_18 = 30000;
    unk_13f0.unk_1a = -1;
    unk_13f0.unk_1c = 0;
    unk_13f0.unk_1e = 0;
    for (int i = 0; i != unk_138b; i++) {
        RegPoolEntry *r = unk_38[i].unk_18;
        if (r)
            r->func_0805a348();
    }
}

#endif

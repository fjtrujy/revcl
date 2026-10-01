// Deferred inline helpers of RegWeb; emitted out of line via #pragma implementation in RegWeb.cpp.
#ifndef REGWEB_INLINE_H
#define REGWEB_INLINE_H

#pragma interface

#include "RegWeb.h"

inline bool resetRegWebs()
{
    regWebCount = 0;
    regWebUsed = 0;
    return true;
}

inline bool RegWeb::AddiTable(workTable *t)
{
    if (!t) {
        func_0804ad38("RegWeb::AddiTable - passed a null pointer\n");
        return false;
    }
    for (Instr *in = t->unk_88.unk_00; in; in = in->unk_1c.unk_00) {
        if (!AddiInstr(in)) {
            func_0804ad38("RegWeb::AddiTable: failed to add an isn\n");
            in->func_08054008(false);
            t->dump();
            return false;
        }
    }
    return true;
}

#endif

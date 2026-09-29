// End-of-table patch helper used by the tidy pass; emitted via #pragma implementation in tidy.cpp.
#ifndef ENDPATCH_H
#define ENDPATCH_H

#pragma interface

#include "workTable.h"
#include "os.h"

inline bool extendForEnd(workTable *wt)
{
    do {
        int i = wt->unk_34++;
        wtRow *r = wt->func_080724f0(i);
        if (!r) {
            func_0804ad38("failed to get ie for end patch\n");
            return false;
        }
        r->func_080721f8();
    } while (wt->unk_34 - wt->unk_30 < 2);
    return true;
}

#endif

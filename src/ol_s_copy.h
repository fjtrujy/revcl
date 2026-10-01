// ol_s copy helper, only visible to olList.cpp (emitted there out of line).
#ifndef OL_S_COPY_H
#define OL_S_COPY_H
#pragma interface

#include "ol_s_inline.h"

inline void ol_s::func_08058054(ol_s *src)
{
    reset();
    if (src) {
        unk_13 = src->unk_13;
        unk_04 = src->unk_04;
        unk_10 = src->unk_10;
        unk_11 = src->unk_11;
        unk_12 = src->unk_12;
    }
}

inline void ol_s::init(Instr *inst, int op, int slot, bool ext)
{
    reset();
    if (ext)
        unk_13 = 1;
    unk_04 = inst;
    unk_10 = op;
    unk_11 = slot;
}

#endif

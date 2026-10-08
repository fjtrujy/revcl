// Inline ol_s members, kept out of the #pragma interface header.
#ifndef OL_S_INLINE_H
#define OL_S_INLINE_H

#include "ol_s.h"

inline void ol_s::reset()
{
    unk_13 = 0;
    unk_04 = 0;
    unk_10 = -1;
    unk_12 = 0;
    unk_00 = 0;
    unk_11 = 0;
    unk_0c = 0;
    unk_08 = 0;
}

inline bool ol_s::isExt()
{
    return unk_13 & 1;
}

inline bool ol_s::isExtUsed()
{
    return flags.externalUsed;
}

inline ol_s::ol_s()
{
    reset();
}

inline ol_s::~ol_s()
{
}

#endif

// Inline ol_s members, kept out of the #pragma interface header.
#ifndef OL_S_INLINE_H
#define OL_S_INLINE_H

#include "ol_s.h"

inline void ol_s::reset()
{
    stateFlags = 0;
    instruction = 0;
    operandIndex = -1;
    unk_12 = 0;
    next = 0;
    componentIndex = 0;
    unk_0c = 0;
    pathKey = 0;
}

inline bool ol_s::isExt()
{
    return stateFlags & 1;
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

// Inline ol_s members, kept out of the #pragma interface header.
#ifndef OL_S_INLINE_H
#define OL_S_INLINE_H

#include "ol_s.h"

inline void OperandLocation::reset()
{
    stateFlags = 0;
    instruction = 0;
    operandIndex = -1;
    cycleOffset = 0;
    next = 0;
    componentIndex = 0;
    maximumPathKey = 0;
    pathKey = 0;
}

inline bool OperandLocation::isExt()
{
    return stateFlags & 1;
}

inline bool OperandLocation::isExtUsed()
{
    return flags.externalUsed;
}

inline OperandLocation::OperandLocation()
{
    reset();
}

inline OperandLocation::~OperandLocation()
{
}

#endif

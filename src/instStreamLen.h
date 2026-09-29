// Inline instStream length accessors, emitted out of line by lock.cpp.
#ifndef INSTSTREAMLEN_H
#define INSTSTREAMLEN_H
#pragma interface

#include "instStream.h"

inline int instStream::func_0805f284()
{
    int len = unk_2e0;
    if (len <= 0)
        len = 0;
    return len;
}

inline int instStream::func_0805f29c()
{
    int len = unk_2e4;
    if (len <= 0)
        len = 0;
    return len;
}

#endif

// fdep link helper; inline, emitted out-of-line via #pragma implementation in fDep.cpp.
#ifndef FDEP_H
#define FDEP_H

#pragma interface

#include "os.h"
#include "Instr.h"
#include "ol_s.h"

inline bool fdep_Link(Instr *pre, Instr *post)
{
    if (!pre && !post) {
        func_0804ad38("fdep_Link -passed a null ptr\n");
        return false;
    }
    if (!pre->unk_13a8.func_0805826c(post)) {
        func_0804ad38("failed to add post to the post list\n");
        return false;
    }
    if (!post->unk_13a0.func_0805826c(pre)) {
        func_0804ad38("failed to add pre to the pre list\n");
        return false;
    }
    return true;
}

#endif

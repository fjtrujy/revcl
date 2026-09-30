// Work table row slot: a tagged Instr * (low bit set = borrowed, not owned).
// Inline members are emitted out of line via #pragma implementation in loadStalls.cpp.
#ifndef WTENTRY_H
#define WTENTRY_H
#pragma interface

#include "Instr.h"

bool addFreeMove(Instr *in);

class wtEntry {
public:
    Instr *unk_00;

    wtEntry() { unk_00 = 0; }
    ~wtEntry()
    {
        if (unk_00 && !((int)unk_00 & 1)) {
            if (unk_00->unk_1428)
                addFreeMove(unk_00);
            else
                delete unk_00;
        }
        unk_00 = 0;
    }
    bool func_08072218() { unk_00 = 0; return 1; }
};

#endif

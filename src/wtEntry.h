// Work table row slot: a tagged Instr * (low bit set = borrowed, not owned).
// Inline members are emitted out of line via #pragma implementation in loadStalls.cpp.
#ifndef WTENTRY_H
#define WTENTRY_H

#include "Instr.h"
#include "vcl_portability.h"

bool addFreeMove(Instr *in);

class wtEntry {
public:
    Instr *unk_00;

    wtEntry();
    ~wtEntry();
    bool func_08072218();
};

struct wtEntryInl {
    Instr *unk_00;

    wtEntryInl() { unk_00 = 0; }
    ~wtEntryInl()
    {
        if (unk_00 && !((VclPtrWord)unk_00 & 1)) {
            if (unk_00->unk_1428)
                addFreeMove(unk_00);
            else
                delete unk_00;
        }
        unk_00 = 0;
    }
    bool func_08072218() { return ((wtEntry *)this)->func_08072218(); }
    operator wtEntry *() { return (wtEntry *)this; }
};

#endif

// Work table row slot: a tagged Instr * (low bit set = borrowed, not owned).
// Inline members are emitted out of line via #pragma implementation in loadStalls.cpp.
#ifndef WTENTRY_H
#define WTENTRY_H

#include "Instr.h"
#include "vcl_portability.h"

bool addFreeMove(Instr *in);

class wtEntry {
public:
    Instr *instruction;

    wtEntry();
    ~wtEntry();
    bool clearInstruction();
};

struct wtEntryInl {
    Instr *instruction;

    wtEntryInl() { instruction = 0; }
    ~wtEntryInl()
    {
        if (instruction && !((VclPtrWord)instruction & 1)) {
            if (instruction->moveRecordWord)
                addFreeMove(instruction);
            else
                delete instruction;
        }
        instruction = 0;
    }
    bool clearInstruction() { return ((wtEntry *)this)->clearInstruction(); }
    operator wtEntry *() { return (wtEntry *)this; }
};

#endif

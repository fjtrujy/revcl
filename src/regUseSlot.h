// Per-slot register use data, embedded in regUse (names guessed).
#ifndef REGUSESLOT_H
#define REGUSESLOT_H

#pragma interface

#include "Instr.h"
#include "tPairList.h"

struct regUseSlot {
    olList unk_00;
    olList unk_08;
    tPairList unk_10;
    olList unk_18;
    olList unk_20;
    olList unk_28;
    short unk_30;
    short unk_32;
    short unk_34[4];
    short unk_3c[13];
    int unk_58;
    cls0805bcc4 unk_5c;

    regUseSlot();
    ~regUseSlot() {}
};

#endif

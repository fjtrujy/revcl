// Per-register back-trace reference (group 5/7). Layout partly known.
#ifndef REGREF_H
#define REGREF_H

#include "tPairList.h"

struct RegPoolEntry;

struct regRefPart {             // sizeof == 0xac
    tPairList unk_00;
    char unk_08[0xa4];
};

struct regRef {
    int unk_00;
    int unk_04;
    RegPoolEntry *unk_08;       // register
    char unk_0c[0x24];
    regRefPart unk_30[1];       // real count unknown
};

#endif

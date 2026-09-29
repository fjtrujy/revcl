// Back-trace node used when building t pairs (group 5/7). Layout partly known.
#ifndef TRACENODE_H
#define TRACENODE_H

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

struct traceNode {
    char unk_00[0x108];
    traceNode *unk_108;         // next in Program::unk_a0
    char unk_10c[0x128];
    regRef *unk_234;
    char unk_238[4];
    int unk_23c;                // dst key
    int unk_240;
    int unk_244;
    int unk_248;
    int unk_24c;
    traceNode *unk_250;         // next in Program::unk_dc
    char unk_254[8];
    traceNode *unk_25c;         // next in Program::unk_e4
};

#endif

#ifndef REGPOOL_H
#define REGPOOL_H

#include "mList.h"

struct RegPoolSub {
    ~RegPoolSub() {}

    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0c;
    int unk_10;
};

struct RegPoolSlot {
    int unk_00[0x4c / 4];
    RegPoolSub unk_4c[4];
};

struct RegPoolEntry {
    int unk_00;
    int unk_04;
    RegPoolEntry* next;
    int unk_0c[(0x44 - 0x0c) / 4];
    RegPoolSlot unk_44[4];
    int unk_2b4;
    mList unk_2b8;
};

class RegPool {
public:
    RegPool();
    virtual ~RegPool();

    RegPoolEntry* unk_04;
    int unk_08;
    short unk_0c;
    short unk_0e;
    int unk_10;
    int unk_14;
    int unk_18;
    RegPoolEntry* unk_1c;
    RegPoolEntry* unk_20;
    int unk_24;
    int unk_28;
    int unk_2c;
};

#endif

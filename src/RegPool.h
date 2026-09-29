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

class RegPool;

struct RegPoolEntry {
    int unk_00;
    RegPool *unk_04;    // owning pool
    RegPoolEntry* next;
    const char* func_0805a8e0();

    int unk_0c;
    int unk_10;
    bool unk_14;        // virtual
    bool unk_15;        // allocable
    bool unk_16;
    char unk_17;
    int unk_18[4];
    int unk_28;         // slot count
    int unk_2c;
    int unk_30;         // register index
    int unk_34[(0x44 - 0x34) / 4];
    RegPoolSlot unk_44[4];
    int unk_2b4;
    mList unk_2b8;
};

class RegPool {
public:
    RegPool();
    virtual ~RegPool();
    RegPoolEntry* func_080593e0(int index);
    RegPoolEntry* func_08059418(const char* name);
    bool func_08059c00();

    RegPoolEntry* unk_04;
    int unk_08;
    short unk_0c;
    short unk_0e;
    RegPoolEntry* unk_10;   // zero register
    int unk_14;
    int unk_18;
    RegPoolEntry* unk_1c;
    RegPoolEntry* unk_20;
    int unk_24;
    int unk_28;
    int unk_2c;
};

#endif

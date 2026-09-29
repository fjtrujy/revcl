// Work table: per-register use lists (names guessed), CIE group 6 owns the lookup.
#ifndef WORKTABLE_H
#define WORKTABLE_H

#include "Instr.h"
#include "tPairList.h"
#include "regUseSlot.h"

struct ol_s;
struct RegPoolEntry;

struct regUse {
    regUse *unk_00;          // prev
    regUse *unk_04;          // next
    RegPoolEntry *unk_08;    // register
    bool unk_0c;
    bool unk_0d;
    bool unk_0e;
    bool unk_0f;
    bool unk_10;
    bool unk_11;
    bool unk_12;
    bool unk_13;
    bool unk_14;
    char unk_15;
    char unk_16[2];
    short unk_18;
    int unk_1c;
    regUseSlot unk_20[4];

    regUse(RegPoolEntry *reg);
    ~regUse();
    void func_0805b51c();
};

class regUseList {
public:
    regUseList();
    ~regUseList();
    bool clear();
    regUse *AddNewRegRef(RegPoolEntry *reg);
    bool append(regUse *ru);
    bool remove(regUse *ru);
    bool func_0805b0cc();
    bool func_0805b0f8();
    void func_0805b44c();
    int func_0805b494(int regClass);
    regUse *func_0805b5dc(RegPoolEntry *reg);

    regUse *find(RegPoolEntry *reg)
    {
        if (!reg)
            return 0;
        regUse *p = unk_04;
        while (p) {
            if (reg == p->unk_08)
                break;
            p = p->unk_04;
        }
        return p;
    }

    int unk_00;      // count
    regUse *unk_04;  // head
    regUse *unk_08;  // tail
};

class workTable {
public:
    char unk_00[0x14];
    regUseList unk_14;
};

#endif

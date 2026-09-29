// Work table: per-register use lists (names guessed), CIE group 6 owns the lookup.
#ifndef WORKTABLE_H
#define WORKTABLE_H

struct ol_s;
struct RegPoolEntry;

struct regUseSlot {
    ol_s *unk_00;  // list of ol_s
    char unk_04[0xac - 4];
};

struct regUse {
    int unk_00;
    regUse *unk_04;          // next
    RegPoolEntry *unk_08;    // register
    char unk_0c[0x20 - 0x0c];
    regUseSlot unk_20[4];
};

class regUseList {
public:
    regUse *func_0805b5dc(RegPoolEntry *reg);

    int unk_00;
    regUse *unk_04;  // head
};

class workTable {
public:
    char unk_00[0x14];
    regUseList unk_14;
};

#endif

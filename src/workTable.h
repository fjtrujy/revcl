// Work table: per-register use lists (names guessed), CIE group 6 owns the lookup.
#ifndef WORKTABLE_H
#define WORKTABLE_H

#include "Instr.h"
#include "tPairList.h"
#include "regUseSlot.h"

struct ol_s;
class instStream;
struct RegPoolEntry;

struct regUse {
    regUse *unk_00;          // prev
    regUse *unk_04;          // next
    RegPoolEntry *unk_08;    // register
    unsigned char unk_0c;    // component masks
    unsigned char unk_0d;
    unsigned char unk_0e;
    unsigned char unk_0f;
    unsigned char unk_10;
    bool unk_11;
    unsigned char unk_12;    // hazard bits per part
    unsigned char unk_13;
    bool unk_14;
    char unk_15;
    short unk_16;
    short unk_18;
    int unk_1c;              // defining instruction (Instr *)
    regUseSlot unk_20[4];

    regUse(RegPoolEntry *reg);
    ~regUse();
    void func_0805b51c();
    regUse *nextReal();
    regUse *nextVirtual();
    bool optReset();
    regUse *Clone();
    const char *regName();

    bool isExternal() { return unk_0c || unk_0d || unk_10; }
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
    int func_0805ba64(int regClass);
    void func_0805bab0();
    void func_0805bb10(int regClass);
    regUse *func_0805b5dc(RegPoolEntry *reg);
    regUse *firstReal();
    regUse *firstVirtual();
    bool optReset();

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

struct wtRow {
    Instr *unk_00[2];
    char unk_08[8];         // unk_08[0]: m_bSkip

    wtRow();
    ~wtRow();
    bool func_080721f8();  // clears unk_00
    bool func_0807222c(int);
    int func_08072304();   // count of used entries
};

struct wtInstrList {
    Instr *unk_00;          // head
    Instr *unk_04;          // tail
};

class workTable {
public:
    char unk_00[8];
    char unk_08;
    unsigned char unk_09;   // code mode
    unsigned char unk_0a;
    char unk_0b;
    int unk_0c;
    instStream *unk_10;     // owner
    regUseList unk_14;
    char unk_20[8];
    int unk_28;             // row count
    wtRow *unk_2c;          // rows
    int unk_30;             // range start
    int unk_34;             // range end (one past)
    int unk_38;             // lsz
    int unk_3c;             // mlid
    char unk_40[4];
    int unk_44;             // extminlength
    char unk_48[0x88 - 0x48];
    wtInstrList unk_88;     // linear list
    int unk_90;             // linear count
    char unk_94[0xc8 - 0x94];
    int unk_c8;             // instruction count in table

    wtRow *func_080724f0(int i);
    workTable();
    ~workTable();
    Instr *getBranch();
    bool takeOut(Instr *i);
    bool putIn(Instr *i);
    int func_08072eec();
    int getLogicalLen();
    int getPhysicalLen();
    void func_08072c70();
    const char *ownerName();
    bool func_08073854();
    int func_08073754();
    int func_0807411c();    // range start
    int func_080741b4();    // lsz
    int func_080741c0();    // range end
    bool grow(int n);
    bool replaceInstruction(Instr *old, Instr *nw);
    bool destroyInstruction(Instr *&in);
    bool prepRawTable();
    bool func_08073774(unsigned char mode);
    bool func_080728fc(unsigned char mode, int size);  // init table (name guessed)
    bool changeMode(unsigned char mode);
};

#endif

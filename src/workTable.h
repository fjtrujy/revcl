// Work table: per-register use lists (names guessed), CIE group 6 owns the lookup.
#ifndef WORKTABLE_H
#define WORKTABLE_H

#include "Instr.h"
#include "tPairList.h"
#include "regUseSlot.h"
#include "wtEntry.h"

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
    wtEntry unk_00[2];
    char unk_08;            // m_bSkip
    unsigned char unk_09;   // max stall length
    unsigned char unk_0a;   // stall type mask
    char unk_0b;
    short unk_0c;

    wtRow();
    ~wtRow();
    bool func_080721f8();  // clears unk_00
    void func_0807222c(int);
    Instr *getEntry(int p);
    int func_08072304();   // count of used entries
    Instr *onlyEntry();
    bool clearStalls();
    void func_08072364();
    bool addStall(int type, int len);
    int func_080723ec(RegPoolEntry *reg, unsigned char mask);
    int func_08072440(RegPoolEntry *reg, unsigned char mask);
    unsigned int func_08072494();
};

struct wtInstrList {
    Instr *unk_00;          // head
    Instr *unk_04;          // tail
    int unk_08;             // count

    wtInstrList() { unk_04 = 0; unk_08 = 0; unk_00 = 0; }
    void reset() { unk_04 = 0; unk_08 = 0; unk_00 = 0; }
    ~wtInstrList() { unk_04 = 0; unk_00 = 0; unk_08 = 0; }
};

inline const char *codeModeStr(int mode)
{
    switch (mode) {
    case 0: return "CODE_MODE_BAD";
    case 1: return "CODE_MODE_UTA";
    case 2: return "CODE_MODE_MID";
    case 3: return "CODE_MODE_RAW";
    case 4: return "CODE_MODE_SYNTH";
    case 5: return "CODE_MODE_RAW_PREEXP";
    case 6: return "CODE_MODE_PASS_THRU";
    }
    return "CODE_MODE_????";
}

class workTable {
public:
    int unk_00;
    char unk_04[4];
    char unk_08;
    unsigned char unk_09;   // code mode
    unsigned char unk_0a;
    char unk_0b;
    int unk_0c;
    instStream *unk_10;     // owner
    regUseList unk_14;
    int unk_20;
    cls0805c244 *unk_24;    // lock lists
    int unk_28;             // row count
    wtRow *unk_2c;          // rows
    int unk_30;             // range start
    int unk_34;             // range end (one past)
    int unk_38;             // lsz
    int unk_3c;             // mlid
    int unk_40;
    int unk_44;             // extminlength
    char unk_48[4];
    int unk_4c;
    int unk_50;
    cls08058718 unk_54;
    cls0805e2e4 unk_5c;
    char unk_74[4];
    short unk_78;
    short unk_7a;
    char unk_7c[0x88 - 0x7c];
    wtInstrList unk_88;     // linear list
    wtInstrList unk_94;
    wtInstrList unk_a0;
    char unk_ac;
    char unk_ad;
    char unk_ae[2];
    int unk_b0;             // move count, class 3
    int unk_b4;             // move count, odd class
    int unk_b8;             // move count, even class
    int unk_bc;
    int unk_c0[3];          // per-pipe instr counts, [2] = total in table

    wtRow *func_080724f0(int i);
    wtRow *rowAt(int i)
    {
        wtRow *r;
        if (!unk_2c)
            r = 0;
        else if (i < 0 || i >= unk_28)
            r = 0;
        else
            r = &unk_2c[i];
        return r;
    }
    workTable();
    ~workTable();
    Instr *getBranch();
    bool removeBranch();
    bool logUnlinkFromLinear(Instr *in);
    int getCondLid();
    void dumpSynthetics();
    bool takeOut(Instr *i);
    bool putIn(Instr *i);
    int func_08072eec();
    int getLogicalLen();
    int getPhysicalLen();
    void dump();
    bool reserveEntry(int key, int row, int slot);
    Instr *func_08073060();
    const char *ownerName();
    bool func_08073854();
    bool dispose();
    bool func_080737e4();
    char *emit(char *buf, int a, int *p1, int *p2);
    int func_08073754();
    int func_0807411c();    // range start
    int func_080741b4();    // lsz
    int func_080741c0();    // range end
    bool grow(int n);
    bool replaceInstruction(Instr *old, Instr *nw);
    bool destroyInstruction(Instr *&in);
    bool prepRawTable();
    bool func_08073774(unsigned char mode);
    bool init(unsigned char mode, int size);
    bool changeMode(unsigned char mode);
    bool clearClones();
    Instr *getInstr(int rl, int p);
    Instr *getInstrInRange(int rl, int p);
    int getPipeCount(int p);
};

const char *codeModeName(int mode);
const char *rawModeName(int mode);
const char *allocModeName(unsigned int mode);

bool cloneAppend(workTable *from, workTable *to, int lidS, int lidE, bool a5, bool a6, bool a7);
extern Instr *g_080de72c;   // instr with smallest unk_1396
void func_080737b8(Instr *in);

#endif

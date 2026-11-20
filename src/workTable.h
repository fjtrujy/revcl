// Work table: per-register use lists (names guessed), CIE group 6 owns the lookup.
#ifndef WORKTABLE_H
#define WORKTABLE_H

#ifndef WORK_TABLE_CTOR_EMISSION
#define WORK_TABLE_CTOR_EMISSION
#endif

#include "os.h"
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
    unsigned char unk_14;    // hazard broadcast bits per part
    char unk_15;
    short unk_16;
    short unk_18;
    Instr *unk_1c;           // defining instruction
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
    int ctime() { return unk_0c; }
    int func_08072304();   // count of used entries
    Instr *onlyEntry();
    bool clearStalls();
    void func_08072364();
    bool addStall(int type, int len);
    int func_080723ec(RegPoolEntry *reg, unsigned char mask);
    int outputMask(RegPoolEntry *reg) { return func_080723ec(reg, 0xff); }
    int func_08072440(RegPoolEntry *reg, unsigned char mask);
    unsigned int func_08072494();
    char *emit(char *buf, int flags, bool upper, bool unused);
};

struct wtInstrList {
    Instr *unk_00;          // head
    Instr *unk_04;          // tail
    int unk_08;             // count

    wtInstrList() { unk_04 = 0; unk_08 = 0; unk_00 = 0; }
    void reset() { unk_04 = 0; unk_08 = 0; unk_00 = 0; }
    Instr *first() { return unk_00; }
    Instr *last() { return unk_04; }
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
    short unk_04;
    char unk_06[2];
    char unk_08;
    unsigned char unk_09;   // code mode
    unsigned char unk_0a;
    bool isAbstract() { return unk_09 == 1; }
    unsigned char codeMode() { return unk_09; }
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
    Instr *unk_4c;          // move list head (as Block::unk_4c)
    int unk_50;
    cls08058718 unk_54;
    cls0805e2e4 unk_5c;
    short unk_74;
    short unk_76;
    short unk_78;
    short unk_7a;
    short unk_7c;
    char unk_7e[2];
    short unk_80;
    short unk_82;
    Instr *unk_84;
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
    workTable() WORK_TABLE_CTOR_EMISSION;
    ~workTable();
    Instr *getBranch();
    bool removeBranch();
    bool unlinkAndUnlog(Instr *in);
    bool logUnlinkFromLinear(Instr *in);
    int getCondLid();
    void dumpSynthetics();
    bool takeOut(Instr *i);
    bool putIn(Instr *i);
#ifdef VCL_POSTOPT_FLOAT_LENGTH
    float func_08072eec();
#else
    int func_08072eec();
#endif
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
    void resetCounts();
    int getCurrentDoneSize();
    bool getCurrentLidStats(int *maxLid, int *maxBranchLid, int *minSfx1, int *minSfx2);
    bool func_08073f90();
    bool setRange();
    bool setRange(int len);
    bool setRange(int s, int e);
    bool setSkip(int row, char skip);
    bool logAddLinear_After(Instr *after, Instr *n);
    bool logAddLinear_Head(Instr *n);
    bool logAddLinear_Tail(Instr *n);
    bool makeLinearList();
    bool relinkRawIt();
    bool lockLine(cls08058718 *line);
    bool unlockLine(cls08058718 *line);
    bool qRegRef(regUse *ru);
    cls0805c244 *getLockList(RegPoolEntry *reg, int part);
    bool qInstruction(Instr *in);
    bool forceInsert(Instr *in, cls08058718 *pos);
    bool doPostOpt(int rawMode);
    workTable *localClone();
    int ctime(int l)
    {
        if (l < unk_30 || l >= unk_34)
            func_0804ae34("nop line is bad\n");
        return unk_2c[l].unk_0c;
    }
    void clearLinear()
    {
        unk_88.reset();
        unk_c0[2] = 0;
        unk_bc = 0;
        unk_b0 = 0;
        unk_b4 = 0;
        unk_c0[0] = 0;
        unk_b8 = 0;
        unk_c0[1] = 0;
    }
};

const char *codeModeName(int mode);
const char *rawModeName(int mode);
const char *allocModeName(unsigned int mode);

bool cloneAppend(workTable *from, workTable *to, int lidS, int lidE, bool a5, bool a6, bool a7);
extern Instr *g_080de72c;   // instr with smallest unk_1396
void func_080737b8(Instr *in);

#endif

#ifndef INSTR_H
#define INSTR_H

#include "RegPool.h"
#include "mValue.h"
#include "mList.h"
#include "os.h"

class instStream;
class LocTag;
class Instr;
struct ol_s;
struct Operand;
struct regUse;
class RegWeb;

// Opcode descriptor ("iInfo", from the null-argument error string).
struct iOpDesc {
    unsigned char unk_00;   // operand kind, 0 terminates the list
    unsigned char unk_01;   // direction: 1 = input, 2 = output
    unsigned char unk_02;   // implicit operand
    unsigned char unk_03;
    unsigned char unk_04;
    unsigned char unk_05;
    unsigned char unk_06;   // patch: paired operand index + 1
};

class iInfo {
public:
    char *unk_00;
    char *unk_04;           // mnemonic
    char *unk_08;
    unsigned char unk_0c;
    char unk_0d;
    union {
        unsigned short unk_0e_w;
        struct {
            char unk_0e;
            char unk_0f;
        };
    };
    char unk_10;
    char unk_11;
    char unk_12;
    char unk_13[5];
    unsigned char unk_18;
    unsigned char unk_19;
    unsigned char unk_1a;   // branch target operand index + 1
    char unk_1b;
    int unk_1c;             // EFU mod-bits type (efu_Hook)
    int (*unk_20)(void *, Instr *);     // per-opcode handler
    char unk_24[0x34 - 0x24];
    char *unk_34;           // mnemonic of the sense-reversed instruction
    char *(*unk_38)(char *buf, Instr *in, bool upper, char *mods, bool b, int zero); // emit hook
    char unk_3c[0x44 - 0x3c];
    iOpDesc unk_44[12];     // operand descriptors

    unsigned char kind() { return unk_18 & 7; }

    unsigned char func_0806ffb4();
    unsigned char ramType();
    unsigned char func_08070020();
    bool func_08070044();
    char func_0807005c();
    void func_08070080();
    void func_080700a4();
    bool check();
    int func_080701f4();
    int func_0807021c();
    int func_08070244();
    int func_0807026c();
};

// List of ol_s nodes returned to a free pool (class name guessed).
class olList {
public:
    ol_s *unk_00;   // head
    int unk_04;

    olList()
    {
        unk_00 = 0;
        unk_04 = 0;
    }
    ~olList() { clear(); }
    bool clear();
    void func_08057e9c();
    void func_08057f2c();
    ol_s *func_080580fc(Instr *inst, int op, int slot, bool ext);
    ol_s *func_0805826c(Instr *inst);
    ol_s *func_08058348(ol_s *ol, bool ext);
    ol_s *func_08058370();
    ol_s *func_080583d8();
    ol_s *func_08058484(ol_s *src);
};

// Base of OpSlot: fields 0x00..0x4f reset by func_0805bcc4 (class name guessed).
class cls0805c244;
class cls08058718;

class cls0805bcc4 {
public:
    unsigned char unk_00;
    unsigned char unk_01;
    unsigned char unk_02;
    cls0805c244 *unk_04;    // owning locklist
    cls0805bcc4 *unk_08;    // next sibling in parent's child list
    cls0805bcc4 *unk_0c;    // input: parent output lock; output: child list head
    cls0805bcc4 *unk_10;    // next in locklist sorted output list
    unsigned char unk_14;   // 1 = input lock, 2 = output lock
    char unk_15;
    char unk_16;
    char unk_17;
    char unk_18;
    char unk_19;
    char unk_1a;
    char unk_1b;
    cls0805bcc4 *unk_1c;    // in/out partner lock
    short unk_20;
    short unk_22;
    void *unk_24;           // owner: Instr* (oType 1) or regUse* (oType 2)
    unsigned char unk_28;   // part
    unsigned char unk_29;   // operand index
    short unk_2a;
    char unk_2c;
    unsigned char unk_2d;   // owner type: 0 none, 1 Instr, 2 regUse
    short unk_2e;
    short unk_30;
    short unk_32;
    short unk_34;
    short unk_36;
    short unk_38;
    short unk_3a;
    short unk_3c;
    short unk_3e;
    short unk_40;
    short unk_42;
    short unk_44;
    short unk_46;
    short unk_48;
    short unk_4a;
    short unk_4c;
    short unk_4e;

    cls0805bcc4() { func_0805bcc4(); }
    ~cls0805bcc4() {}
    bool func_0805bcc4();
    bool setOwner(int part, Instr *instr, int opnd);
    bool setOwner(int part, regUse *rf);
    bool clearOwner();
    Instr *ownerInstr();
    regUse *ownerRegUse();
    Operand *ownerOperand();
    int ownerOpnd();
    int getPart();
    void printOwner();
    cls0805bcc4 *getParent();
    bool setInOut(cls0805bcc4 *olk);
    bool incRef();
    bool decRef();
    void print();
    bool isOk(int type, bool inserted);
    short func_0805ccc8();
    short func_0805ccd4();
    bool calcStats(Instr *skip);
    void func_0805d020();
    void setFrom(cls08058718 *r);
    short func_0805d0dc();
    const char *regName();
    void resetStats()
    {
        unk_30 = unk_32 = -1;
        unk_34 = -1;
        unk_36 = -1;
        unk_38 = -1;
        unk_3a = -1;
        unk_3c = -1;
        unk_3e = -1;
        unk_40 = -1;
        unk_42 = -1;
        unk_4c = unk_4e = -1;
    }
};

class cls0805c244 {
public:
    short unk_00;
    unsigned char unk_02;   // part
    unsigned char unk_03;   // assigned flag
    RegPoolEntry *unk_04;   // register
    cls0805c244 *unk_08;    // next locklist
    cls0805bcc4 *unk_0c[4]; // external output locks
    cls0805bcc4 *unk_1c;    // output lock list (linked by lock->unk_08)
    cls0805bcc4 *unk_20;    // sorted output list head (by unk_32, linked by unk_10)
    short unk_24;           // nOutLocks
    short unk_26;           // nToGo
    char unk_28;
    char unk_29;
    unsigned char unk_2a;   // extern output count
    char unk_2b;

    cls0805c244();
    cls0805c244(cls0805c244 *next);
    ~cls0805c244();
    void checkCounts();
    bool reset();
    bool addInput(cls0805bcc4 *out, cls0805bcc4 *in, char a, unsigned char b);
    bool addOutput(cls0805bcc4 *lk, int a, int b, unsigned char c);
    bool removeInput(cls0805bcc4 *in);
    bool removeOutput(cls0805bcc4 *lk);
    bool addOutputLock(cls0805bcc4 *lk);
    bool addInputLock(cls0805bcc4 *in);
    bool undoInputLock(cls0805bcc4 *in);
    bool undoOutputLock(cls0805bcc4 *lk);
    int func_0805c938(int n);
    bool isOk(cls0805bcc4 *lk, int type, bool inserted);
    bool moveOutputLock(cls0805bcc4 *lk);
    bool moveInputLock(cls0805bcc4 *in);
    bool addExtOutput(cls0805bcc4 *lk, bool flag);
    Instr *earliestExt();
    int func_0805d3c4();
    bool setRepReg(RegPoolEntry *rg);
    int checkInputLock(cls0805bcc4 *in, short *pos, int *minPos);
    int checkOutputLock(cls0805bcc4 *out, short *pos, int *minPos);
    int checkOverlap(cls0805bcc4 *out, short *pos, int *minPos);
    bool catLocks(cls0805bcc4 *a, cls0805bcc4 *b, int *s, int *e, int *end);
    int canMoveInputLock(cls0805bcc4 *in, short *pos, int slotBit);
    int canMoveOutputLock(cls0805bcc4 *out, short *pos, int slotBit);
};

bool freeLockLists(cls0805c244 *p);
bool func_0805db74(int s1, int e1, int s2, int e2);
cls0805c244 *findLockList(cls0805c244 *p, RegPoolEntry *reg, int part);
const char *rlmtName(unsigned char t);

bool checkTable(iInfo *tab, int count);
int func_08070300(unsigned char);
int func_0807031c(unsigned char);
unsigned char func_08070324(unsigned char);

struct OpSlot : public cls0805bcc4 {
    short unk_50;
    char unk_52[2];
    olList unk_54;          // uses

    OpSlot();
    ~OpSlot();
    void func_080560bc();
    bool func_080560c4();
    int func_080560e8();
};

struct Operand {
    RegWeb *unk_00;         // register web
    char unk_04;            // operand kind
    char unk_05;
    char unk_06;
    unsigned char unk_07;   // register slot mask
    char unk_08;
    char unk_09;            // 1 == register operand
    char unk_0a;
    char unk_0b;
    char unk_0c;
    char unk_0d;            // paired operand index + 1
    char unk_0e[2];
    RegPoolEntry *unk_10;   // forced base register
    RegPoolEntry *unk_14;
    RegPoolEntry *unk_18;   // register
    mValue unk_1c;          // immediate value
    char unk_24;
    char unk_25;
    char unk_26;            // operand index
    unsigned char unk_27;
    char unk_28;
    char unk_29;
    unsigned char unk_2a;   // prune mask
    char unk_2b;
    OpSlot unk_2c[4];       // per register slot

    Operand();
    ~Operand();
    bool func_080563f8();
    void func_0805665c(int);
    char *Emit(char *buf, bool, bool symbolic);
    bool func_080569cc();
    bool func_08056a24();
    Instr *getOwner();
    OpSlot *getPart(int part);
    char *func_08057b90(bool flag, char *buf);
    int getRegFrom(Operand *src);
    const char *func_08057e40();
    RegWeb *GetWeb();
    bool isForwardable()
    {
        return unk_18 && unk_18->unk_04->unk_18 && unk_18 != unk_18->unk_04->unk_10;
    }
    bool isZero()
    {
        int z = 0;
        if (unk_18) {
            if (unk_18 == unk_18->unk_04->unk_10)
                z = 1;
        }
        return z;
    }
    bool isReg()
    {
        bool r = false;
        if (unk_09 == 1 && !isZero())
            r = true;
        return r;
    }
    RegPoolEntry *getReg() { return isReg() ? unk_18 : 0; }
    bool isZeroReg()
    {
        bool z = false;
        if (unk_18) {
            if (unk_18 == unk_18->unk_04->unk_10)
                z = true;
        }
        return z;
    }
    RegPoolEntry *inputReg()
    {
        int r = 0;
        if (unk_09 == 1 && !isZeroReg())
            r = 1;
        return r ? unk_18 : 0;
    }
    RegPoolEntry *outputReg()
    {
        int r = 0;
        if (unk_09 == 2 && !isZero())
            r = 1;
        return (bool)r ? unk_18 : 0;
    }
    RegPoolEntry *inputRegB()
    {
        int r = 0;
        if (unk_09 == 1 && !isZero())
            r = 1;
        return (bool)r ? unk_18 : 0;
    }
    RegPoolEntry *outputRegZ()
    {
        int r = 0;
        if (unk_09 == 2) {
            if (!isZero())
                r = 1;
        }
        return r ? unk_18 : 0;
    }
    RegPoolEntry *outputRegZB()
    {
        int r = 0;
        if (unk_09 == 2 && !isZeroReg())
            r = 1;
        return r ? unk_18 : 0;
    }
    bool isRegOp()
    {
        int r = 0;
        if (unk_09 == 1 && !isZero())
            r = 1;
        return r;
    }
    RegPoolEntry *regOp() { return isRegOp() ? unk_18 : 0; }
    bool isInReg() { return unk_09 == 1 && !isZero(); }
    bool isOutReg() { return unk_09 == 2 && !isZero(); }
    bool isReg2()
    {
        bool r = false;
        if (unk_09 == 2 && !isZero())
            r = true;
        return r;
    }
    bool isOutRegOp()
    {
        int r = 0;
        if (unk_09 == 2 && !isZeroReg())
            r = 1;
        return r;
    }
    bool isInRegOp()
    {
        int r = 0;
        if (unk_09 == 1 && !isZeroReg())
            r = 1;
        return r;
    }
    bool isInRegOpB() { return unk_09 == 1 && !isZeroReg(); }
    bool isInRegB()
    {
        bool r = false;
        if (unk_09 == 1 && !isZeroReg())
            r = true;
        return r;
    }
    RegPoolEntry *outRegZ()
    {
        bool r = false;
        if (unk_09 == 2 && !isZeroReg())
            r = true;
        if (r)
            return unk_18;
        return 0;
    }
    RegPoolEntry *getReg2()
    {
        if (isReg2())
            return unk_18;
        return 0;
    }
    RegPoolEntry *inRegOp()
    {
        bool r = false;
        if (unk_09 == 1 && !isZeroReg())
            r = true;
        return r ? unk_18 : 0;
    }
    bool isZeroReg2()
    {
        bool z = true;
        if (!unk_18 || unk_18 != unk_18->unk_04->unk_10)
            z = false;
        return z;
    }
    RegPoolEntry *inRegOp2()
    {
        bool r = false;
        if (unk_09 == 1 && !isZeroReg2())
            r = true;
        return r ? unk_18 : 0;
    }
    RegPoolEntry *outRegOp()
    {
        bool r = false;
        if (unk_09 == 2 && !isZeroReg())
            r = true;
        return r ? unk_18 : 0;
    }
    bool isZeroB()
    {
        int z = 0;
        if (unk_18 && unk_18 == unk_18->unk_04->unk_10)
            z = 1;
        return z;
    }
    RegPoolEntry *outputRegB()
    {
        int r = 0;
        if (unk_09 == 2 && !isZeroB())
            r = 1;
        return (bool)r ? unk_18 : 0;
    }
    RegPoolEntry *inputRegC()
    {
        int r = 0;
        if (unk_09 == 1 && !isZeroB())
            r = 1;
        return (bool)r ? unk_18 : 0;
    }
    bool isZeroC() { return unk_18 && unk_18 == unk_18->unk_04->unk_10; }
    bool isInRegC() { return unk_09 == 1 && !isZeroC(); }
    bool isOutRegC() { return unk_09 == 2 && !isZeroC(); }
    bool isHardZero()
    {
        if (unk_18 && unk_18 == unk_18->unk_04->unk_10)
            return true;
        return false;
    }
    bool isOutputReg()
    {
        int r = 0;
        if (unk_09 == 2 && !isHardZero())
            r = 1;
        return (bool)r;
    }
    bool isInputReg()
    {
        int r = 0;
        if (unk_09 == 1 && !isHardZero())
            r = 1;
        return (bool)r;
    }
    bool isInput() { return unk_09 == 1; }
    RegPoolEntry *inReg()
    {
        if (unk_09 == 1)
            return unk_18;
        return 0;
    }
    RegPoolEntry *outReg()
    {
        if (unk_09 == 2)
            return unk_18;
        return 0;
    }
};

class Instr;

// Doubly linked list link embedded in Instr.
struct iLink {
    Instr *unk_00;  // next
    Instr *unk_04;  // prev

    Instr *prev() { return unk_04; }

    iLink()
    {
        unk_04 = 0;
        unk_00 = 0;
    }
    ~iLink()
    {
        unk_04 = 0;
        unk_00 = 0;
    }
};

#include "iGroupList.h"

class cls08058718 {
public:
    short unk_00;
    char unk_02;
    char unk_03;
    short unk_04;
    char unk_06[2];

    cls08058718();
    ~cls08058718();
    void func_080586e0();
    void func_08058744();
    void func_0805874c(short v);
    void func_0805878c(char c);
    short getRow() { return unk_00; }
    short getPipe() { return unk_02; }
    void setRow(short r)
    {
        unk_03 = 0;
        unk_00 = unk_04 = r;
        unk_02 = 1;
    }
};

extern int g_080b336c;  // range lsz
extern bool g_080de284; // rfUsed before result ready is fatal (name guessed)
extern bool g_080de288;
extern int g_080b3370;  // range start
extern int g_080b3374;  // range end

class cls0805e2e4 {
public:
    short unk_00, unk_02, unk_04, unk_06;
    int unk_08, unk_0c, unk_10, unk_14;

    cls0805e2e4();
    ~cls0805e2e4();
    void func_0805e2fc();
    void func_0805e304();
    void func_0805e340();
    void func_0805e37c(cls0805e2e4 *src);
    void func_0805e3c0(cls0805e2e4 *o);
};

// Placeholder for the object at Instr::unk_30 (type unknown).
struct cls_unk30 {
    char unk_00[9];
    char unk_09;
    char unk_0a[6];
    instStream *unk_10;     // owner stream

    Instr **getIE_ck(int rl);
};

class Instr {
public:
    int unk_00;
    int unk_04;
    Instr *unk_08;  // tmpClone
    short unk_0c;   // id
    union {
        unsigned char unk_0e;
        struct {
            unsigned char unk_0e_0 : 1;
            unsigned char unk_0e_1 : 1;
        };
    };
    char unk_0f;
    iGroupList unk_10;
    LocTag *unk_14;
    short unk_18;
    short unk_1a;
    iLink unk_1c;   // linear list link
    iLink unk_24;   // ToDo/Done list link
    Instr *unk_2c;  // next in Block move list
    cls_unk30 *unk_30;
    iInfo *unk_34;  // opcode descriptor
    Operand unk_38[12];
    unsigned char unk_1388;     // flags
    char unk_1389;
    bool unk_138a;
    unsigned char unk_138b;     // operand count
    char unk_138c;
    unsigned char unk_138d;
    char unk_138e;
    unsigned char unk_138f;
    char unk_1390;
    char unk_1391;
    char unk_1392;
    short unk_1394;
    short unk_1396;
    int unk_1398;
    char *unk_139c;
    olList unk_13a0;
    olList unk_13a8;
    short unk_13b0;
    short unk_13b2;
    short unk_13b4;
    short unk_13b6;
    mList unk_13b8;
    char unk_13cc;
    char unk_13cd;
    char unk_13ce;
    unsigned char unk_13cf;
    unsigned char unk_13d0;
    char unk_13d1;
    short unk_13d2;
    char unk_13d4;
    char unk_13d5;
    char unk_13d6;
    char unk_13d7;
    mValue unk_13d8;
    int unk_13e0;
    int unk_13e4;
    cls08058718 unk_13e8;
    void clearSlot() { unk_13e8.func_080586e0(); }
    cls0805e2e4 unk_13f0;
    short unk_1408, unk_140a, unk_140c, unk_140e;
    short unk_1410;     // fdep "has extra pre deps" flag
    short unk_1412;     // slot mask read of reg idx 48
    short unk_1414;     // slot mask written of reg idx 48
    char unk_1416[2];
    short unk_1418;
    short unk_141a;
    short unk_141c;
    short unk_141e;
    short unk_1420;
    char unk_1422[6];
    int unk_1428;
    short unk_142c;
    short unk_142e;
    short unk_1430;
    short unk_1432;
    Instr *unk_1434;        // prevThruput writer

    bool isConcreteMove()
    {
        return unk_142c && unk_142e;
    }
    bool isAbstractMove()
    {
        return unk_142c && !unk_142e;
    }
    Instr(iInfo *info);
    ~Instr();
    void hasImmValue();
    bool hasImmFixup();
    void resetImmFixup();
    void func_0805f4e0();
    bool isValid()
    {
        if (!unk_34)
            return false;
        for (int i = 0; i != unk_138b; i++)
            if (!unk_38[i].func_080563f8())
                return false;
        return true;
    }
    bool func_08053f4c();
    bool func_08053fac();
    void func_08054008(bool);
    bool func_08054138();
    instStream *func_08054168();  // owner instruction stream
    bool func_0805417c();
    bool func_080541fc();
    bool func_08054214();
    void leDump();
    void leDump(int opnd);
    void leDump(int opnd, int mask);
    bool func_08052f84();
    int func_08053104();
    int func_080532d8();
    int func_08055ac4();
    Operand *getOp(int n)
    {
        if (n < 0) {
            func_0804ad38("-ve operand passed to getOp!\n");
            func_0804adf0(0);
            return 0;
        }
        return n < unk_138b ? &unk_38[n] : 0;
    }
    Instr *clone();
    int getRamBits();
    bool func_08054b84();
    int func_08054c24(RegPoolEntry *reg, bool *paired);
    int func_08054cc4(RegPoolEntry *reg, bool *paired);
    bool func_08054d64(int n, int *out);
    bool changeBranchImmediate(const char *s);
    bool setOwner(cls_unk30 *s);
    char *func_080552a8(bool flag, char *buf, int n);
    int func_080553b4(RegPoolEntry *reg, int mask);
    int func_0805544c(RegPoolEntry *reg, int mask);
    bool func_08055548();
    char *getBranchTargetImmediate();
    RegPoolEntry *getBranchTargetRegister();
    int func_08055718();
    Operand *func_08055720(RegPoolEntry *reg, int mask);
    Operand *func_080557a4(RegPoolEntry *reg, int mask);
    bool func_08055828(unsigned int n);
    bool func_080558f4();
    Instr **getIE();
    int func_08055a0c();
    bool func_08055a88();
    bool func_08055aa8();
    bool func_08055ae4();
    void func_08055f4c();
    void func_08055f78(Instr *src);
    void func_08055fbc(short v);
    void func_08055fdc(short v);
    void func_08055ffc(short v);
    void func_0805601c(short v);
};

iInfo *func_0806bf74(int id);
iInfo *func_0806bf8c(const char *name, bool);
RegPoolEntry *func_0806c224(int idx);
int func_0806c72c(iInfo *op);
Instr *GetI(Instr **ie, int p);
int checkDispatchEntry(Instr **ie);
bool wouldPair(Instr **ie, Instr *in, unsigned p);
bool func_0806c6c4(RegPoolEntry *r);

// Address-alias descriptor per opcode (12-byte entries, names guessed).
struct aaDesc {
    int unk_00;             // opcode id
    char unk_04;            // -> Instr::unk_13d4
    unsigned char unk_05;   // operand index
    unsigned char unk_06;   // output operand index
    signed char unk_07;     // immediate operand index, <0 => unk_08
    int unk_08;             // literal immediate
};

// Address-alias use entry per opcode (8-byte entries, names guessed).
struct aaUse {
    int unk_00;             // opcode id
    unsigned char unk_04;   // operand index
};

aaDesc *func_0806c768(Instr *in);
aaUse *func_0806c7f8(Instr *in);
RegPoolEntry *getOperandZeroReg(int kind);
bool func_080543bc(Instr *a, Instr *b);
bool func_0805449c(Instr *a, Instr *b);
Operand *func_0805802c(ol_s *ol);
void func_0805859c(ol_s *ol);
int func_080585a8(ol_s *ol);
int func_080585c0(ol_s *ol);
bool func_080585d4(ol_s *a, ol_s *b);
instStream *func_08058618(ol_s *ol);
int func_0805862c(ol_s *ol);
ol_s *func_08058640();
int func_080586c4();
bool link_oParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot);
bool link_oParts(ol_s *o, ol_s *i, int x, int y);
const char *func_08055254(int t);
const char *func_080554e4(int t);

// Implicit operand description (name guessed).
struct ImplicitOp {
    char name[0x800];
    char unk_800[0x800];
    char unk_1000[0x800];
    char unk_1800[0x800];
    int unk_2000;           // slot mask
    int unk_2004;
    int unk_2008;
    char unk_200c;
    char unk_200d[3];
};

bool buildImplicitOperand(int kind, ImplicitOp *out, bool);
const char *opKindName(int kind);
int func_08057a34(int kind);
bool func_08057d60(int kind);
bool func_08057d9c(int kind);
const char *func_0806a33c(unsigned char);
extern ol_s *olFreeList;

struct opPair_s {
    int opcode;
    signed char unk_04[4];
    int unk_08;
};

struct opCount_s {
    int opcode;
    int unk_04;
};

extern opPair_s opPairTable_080b32e0[];
extern opCount_s opCountTable_080b3340[];

#endif

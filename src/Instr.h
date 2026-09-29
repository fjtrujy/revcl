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

// Opcode descriptor ("iInfo", from the null-argument error string).
struct iOpDesc {
    char unk_00;    // operand kind, 0 terminates the list
    char unk_01;
    char unk_02;
    char unk_03;
    char unk_04;
    char unk_05;
    char unk_06;    // paired operand index + 1
};

class iInfo {
public:
    char unk_00[4];
    char *unk_04;           // mnemonic
    char unk_08[4];
    unsigned char unk_0c;
    char unk_0d;
    char unk_0e;
    char unk_0f;
    char unk_10;
    char unk_11;
    char unk_12;
    char unk_13[5];
    unsigned char unk_18;
    unsigned char unk_19;
    unsigned char unk_1a;   // branch target operand index + 1
    char unk_1b[0x44 - 0x1b];
    iOpDesc unk_44[1];      // operand descriptors

    char func_0806ffb4();
    char func_0806ffd8();
    char func_08070020();
    bool func_08070044();
    char func_0807005c();
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
    ol_s *func_080580fc(Instr *inst, int op, int slot, bool ext);
    ol_s *func_08058348(ol_s *ol, bool ext);
};

struct OpSlot {
    char unk_00[0x50];
    short unk_50;
    char unk_52[2];
    olList unk_54;          // uses
};

struct Operand {
    char unk_00[4];
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
    int unk_10;
    int unk_14;
    RegPoolEntry *unk_18;   // register
    mValue unk_1c;          // immediate value
    char unk_24;
    char unk_25;
    char unk_26;            // operand index
    unsigned char unk_27;
    char unk_28;
    char unk_29;
    char unk_2a;
    char unk_2b;
    OpSlot unk_2c[4];       // per register slot

    Operand();
    ~Operand();
    bool func_080563f8();
    void func_0805665c(int);
    bool func_080569cc();
    bool func_08056a24();
    char *func_08057b90(bool flag, char *buf);
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
    bool isReg2()
    {
        bool r = false;
        if (unk_09 == 2 && !isZero())
            r = true;
        return r;
    }
    RegPoolEntry *getReg2() { return isReg2() ? unk_18 : 0; }
};

class Instr;

// Doubly linked list link embedded in Instr.
struct iLink {
    Instr *unk_00;  // next
    Instr *unk_04;  // prev

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

// Singly linked list of heap nodes (class name guessed).
class ptrList {
public:
    void *unk_00;   // head

    ptrList();
    ~ptrList();
};

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
};

class cls0805e2e4 {
public:
    char unk_00[0x28];

    cls0805e2e4();
    ~cls0805e2e4();
    void func_0805e37c(cls0805e2e4 *src);
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
    char unk_0c;
    char unk_0d;
    unsigned char unk_0e_0 : 1;
    unsigned char unk_0e_1 : 1;
    char unk_0f;
    ptrList unk_10;
    LocTag *unk_14;
    short unk_18;
    short unk_1a;
    iLink unk_1c;   // linear list link
    iLink unk_24;   // ToDo/Done list link
    int unk_2c;
    cls_unk30 *unk_30;
    iInfo *unk_34;  // opcode descriptor
    Operand unk_38[12];
    unsigned char unk_1388;     // flags
    char unk_1389;
    bool unk_138a;
    unsigned char unk_138b;     // operand count
    char unk_138c;
    char unk_138d;
    char unk_138e;
    unsigned char unk_138f;
    char unk_1390;
    char unk_1391;
    char unk_1392;
    short unk_1394;
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
    char unk_13cf;
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
    cls0805e2e4 unk_13f0;
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
    int unk_1434;

    Instr(iInfo *info);
    ~Instr();
    void func_0806a310();
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
    char *func_080552a8(bool flag, char *buf);
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
};

iInfo *func_0806bf8c(const char *name, bool);
int func_0806c72c(iInfo *op);
RegPoolEntry *func_0806c25c(char kind);
bool func_080543bc(Instr *a, Instr *b);
bool func_0805449c(Instr *a, Instr *b);
Operand *func_0805802c(ol_s *ol);
instStream *func_08058618(ol_s *ol);
bool link_oParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot);
bool link_oParts(ol_s *o, ol_s *i, int x, int y);
const char *func_08055254(int t);
const char *func_080554e4(int t);

#endif

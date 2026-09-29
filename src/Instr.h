#ifndef INSTR_H
#define INSTR_H

#include "RegPool.h"
#include "mValue.h"
#include "mList.h"

class instStream;
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
    char unk_00[0xe];
    char unk_0e;
    char unk_0f;
    char unk_10;
    char unk_11;
    char unk_12;
    char unk_13[5];
    unsigned char unk_18;
    unsigned char unk_19;
    char unk_1a[0x44 - 0x1a];
    iOpDesc unk_44[1];      // operand descriptors

    char func_0806ffb4();
    char func_0806ffd8();
    char func_08070020();
    bool func_08070044();
};

struct Operand {
    char unk_00[4];
    char unk_04;            // operand kind
    char unk_05;
    char unk_06;
    char unk_07;
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
    char unk_27;
    char unk_28;
    char unk_29;
    char unk_2a[0x80 - 0x2a];
    ol_s *unk_80;           // uses
    char unk_84[0x19c - 0x84];

    Operand();
    ~Operand();
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
};

class cls0805e2e4 {
public:
    char unk_00[0x28];

    cls0805e2e4();
    ~cls0805e2e4();
};

class Instr {
public:
    int unk_00;
    int unk_04;
    int unk_08;
    char unk_0c[4];
    ptrList unk_10;
    int unk_14;
    short unk_18;
    short unk_1a;
    iLink unk_1c;   // linear list link
    iLink unk_24;   // ToDo/Done list link
    char unk_2c[4];
    void *unk_30;
    iInfo *unk_34;  // opcode descriptor
    Operand unk_38[12];
    unsigned char unk_1388;     // flags
    char unk_1389;
    bool unk_138a;
    unsigned char unk_138b;     // operand count
    char unk_138c;
    char unk_138d;
    char unk_138e;
    char unk_138f;
    char unk_1390;
    char unk_1391;
    char unk_1392;
    short unk_1394;
    int unk_1398;
    int unk_139c;
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
    void func_0806a310();
    instStream *func_08054168();  // owner instruction stream
    void leDump(int opnd, int mask);
    bool func_08052f84();
    int func_08053104();
    int func_080532d8();
    bool func_08053fac();
    int func_08055ac4();
};

iInfo *func_0806bf8c(const char *name, bool);
int func_0806c72c(iInfo *op);
RegPoolEntry *func_0806c25c(char kind);

#endif

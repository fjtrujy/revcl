#ifndef INSTR_H
#define INSTR_H

#include "RegPool.h"
#include "mValue.h"

class instStream;
struct ol_s;

struct Operand {
    char unk_00[4];
    char unk_04;            // operand kind
    char unk_05[4];
    char unk_09;            // 1 == register operand
    char unk_0a[6];
    int unk_10;
    int unk_14;
    RegPoolEntry *unk_18;   // register
    mValue unk_1c;          // immediate value
    char unk_24[5];
    char unk_29;
    char unk_2a[0x80 - 0x2a];
    ol_s *unk_80;           // uses
    char unk_84[0x19c - 0x84];

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

class Instr {
public:
    char unk_00[0x1c];
    Instr *unk_1c;  // linear list link
    Instr *unk_20;  // linear list link
    Instr *unk_24;  // ToDo/Done list next
    Instr *unk_28;  // ToDo/Done list prev
    char unk_2c[8];
    void *unk_34;   // opcode descriptor
    Operand unk_38[12];
    unsigned char unk_1388;     // flags
    char unk_1389[2];
    unsigned char unk_138b;     // operand count
    char unk_138c[0x13ce - 0x138c];
    char unk_13ce;

    instStream *func_08054168();  // owner instruction stream
    void leDump(int opnd, int mask);
    bool func_08052f84();
    int func_08053104();
    int func_080532d8();
    bool func_08053fac();
    int func_08055ac4();
};

void *func_0806bf8c(const char *name, bool);
int func_0806c72c(void *op);
RegPoolEntry *func_0806c25c(char kind);

#endif

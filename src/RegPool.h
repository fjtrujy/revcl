#ifndef REGPOOL_H
#define REGPOOL_H

#include "mList.h"
#include "os.h"

#include "ol_s.h"

struct rPart {
    short unk_00[0x24 / 2];
    ol_s unk_24;        // current part
    ol_s* unk_38;       // pending source
    short unk_3c;       // age counter
    short unk_3e;       // issueLock
    short unk_40;
    short unk_42;
    short unk_44;
    short unk_46;
    int unk_48;
    ol_s unk_4c[4];     // pending parts

    bool flushPending();
    ol_s* func_0805a164();
    bool cycle(bool flag);
    void func_0805a374();
    void func_0805a3cc();
    void func_0805a4d4();
    void func_0805a4e4();
    rPart();
    int func_0805a948();
    bool func_0805a954(int v);
    ol_s* func_0805a968(int i);
};

class RegPool;
class Instr;

struct RegPoolEntry {
    int unk_00;
    RegPool *unk_04;    // owning pool
    RegPoolEntry* next;
    const char* func_0805a8e0();
    RegPoolEntry();
    void func_0805a300();
    void func_0805a348();
    void func_0805a390();
    mNode* func_0805a8a4(const char* name);
    void func_0805a4b4();
    bool func_0805a4f4(bool flag);
    bool func_0805aa00();
    bool optReset();
    int gatherInflight(ol_s** out);
    void func_0805a340();
    int func_0805a908(int i);
    bool func_0805a924(int i, int v);
    void moveReset();
    rPart* getRP(int i)
    {
        if (i < 0 || i >= unk_28) {
            func_0804ad38("getRP - asked to get out of range part %s.%d / %d\n",
                          func_0805a8e0(), i, unk_28);
            func_0804adf0(-1);
        }
        return &unk_44[i];
    }

    RegPoolEntry* unk_0c;   // prev
    int unk_10;
    bool unk_14;        // virtual
    bool unk_15;        // allocable
    bool unk_16;
    char unk_17;
    int unk_18;
    char unk_1c;        // nonblockid
    char unk_1d[3];
    Instr* unk_20;      // move instruction
    int unk_24;         // register class
    int unk_28;         // slot count (nparts)
    bool unk_2c;
    char unk_2d[3];
    int unk_30;         // register index
    char unk_34;
    unsigned char unk_35;   // used slot mask
    short unk_36;
    int unk_38;
    int unk_3c;
    int unk_40;
    rPart unk_44[4];
    int unk_2b4;
    mList unk_2b8;
};

class RegPool {
public:
    RegPool();
    virtual ~RegPool();
    bool Init(int a, const char* name, int cls, int b);
    void Dump();
    RegPoolEntry* func_080593e0(int index);
    RegPoolEntry* func_08059418(const char* name);
    bool func_08059464();
    bool func_080594c0();
    bool func_0805951c();
    bool func_08059578();
    bool func_080595d4();
    static bool isResvd(const char* name, int cls);
    bool func_08059828(bool flag);
    bool func_080598b0();
    RegPoolEntry* func_08059944(const char* name);
    RegPoolEntry* func_08059a1c(const char* name);
    RegPoolEntry* func_08059a5c(const char* name);
    bool func_08059c00();
    bool func_08059c4c();
    bool func_08059c94();
    bool func_08059cdc();
    RegPoolEntry* getMoveReg(Instr* move);
    bool func_08059e20(RegPoolEntry* e);
    bool optReset();
    bool emitReset();
    char* emitStats(char* buf);
    int regIndex(int reg);
    int countUsed();
    int func_08059f70();
    int gatherInflight(ol_s** out);

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
    const char* unk_2c;     // pool name
};

struct RegDesc {
    const char* names[4];   // aliases; names[0] == 0 ends the table
    unsigned char unk_10;   // allocable
    unsigned char unk_11;   // register class
    unsigned char unk_12;   // nparts
    char unk_13;            // nonblockid
    unsigned char unk_14;
};

extern RegDesc regDescTable[];
extern const char* regNameChars;

int func_0805968c(const char* name);

#endif

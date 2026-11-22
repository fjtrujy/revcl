#ifndef REGPOOL_H
#define REGPOOL_H

#include "vcl_portability.h"

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
    VclPtrWord unk_48;
    ol_s unk_4c[4];     // pending parts

    bool flushPending();
    ol_s* func_0805a164();
    bool cycle(bool flag);
    void func_0805a374();
    void func_0805a3cc();
    void func_0805a4d4();
    void func_0805a4e4();
    rPart();
    VclPtrWord func_0805a948();
    bool func_0805a954(VclPtrWord v);
    ol_s* func_0805a968(int i);
};

class RegPool;
class Instr;
class RegWeb;
class instStream;
class workTable;
struct RegPoolEntry {
    short unk_00;       // min row (getMove)
    short unk_02;       // max row (getMove)
    RegPool *ownerPool;    // owning pool
    RegPoolEntry* next;
    const char* registerName();
    RegPoolEntry();
    void func_0805a300();
    void func_0805a348();
    void func_0805a390();
    mNode* func_0805a8a4(const char* name);
    void func_0805a4b4();
    bool func_0805a4f4(bool flag);
    bool func_0805aa00();
    bool optReset();
    char* maskName(int mask);
    int regIndex();
    int gatherInflight(ol_s** out);
    bool isClass(int cls)
    {
        if (registerClass == cls)
            return true;
        return false;
    }
    void func_0805a340();
    VclPtrWord func_0805a908(int i);
    bool func_0805a924(int i, VclPtrWord v);
    void moveReset();
    rPart* getRP(int i)
    {
        if (i < 0 || i >= partCount) {
            reportError("getRP - asked to get out of range part %s.%d / %d\n",
                          registerName(), i, partCount);
            exitAfterCleanup(-1);
        }
        return &unk_44[i];
    }

    RegPoolEntry* unk_0c;   // prev
    int unk_10;
    bool isVirtual;        // virtual
    bool allocatable;        // allocable
    bool unk_16;
    char unk_17;
    RegPoolEntry* assignedPhysicalRegister;   // allocated physical register
    char unk_1c;        // nonblockid
    char unk_1d[3];
    Instr* unk_20;      // move instruction
    int registerClass;         // register class
    int partCount;         // slot count (nparts)
    bool unk_2c;
    char unk_2d[3];
    int registerIndex;         // register index
    char unk_34;
    unsigned char usedSlotMask;   // used slot mask
    short unk_36;
    Instr *throughputWriter;          // previous thruput writer
    int unk_3c;             // ttab size (nassign cost)
    RegPoolEntry *unk_40;   // next in nassign todo list
    rPart unk_44[4];
    RegWeb *registerWeb;    // register web
    mList unk_2b8;

    RegWeb *GetWeb();
};

class RegPool {
public:
    RegPool();
    virtual ~RegPool();
    virtual bool assignExt(instStream *s) = 0;                        // slot 2
    virtual bool func_08088e6c(instStream *s, RegPoolEntry *reg) = 0; // slot 3
    virtual bool func_08088e54(workTable *t) = 0;                     // slot 4
    bool Init(VclPtrWord registerPrefix, const char* name, int cls, int virtualRegisterParts);
    void Dump();
    RegPoolEntry* findByRegisterIndex(int index);
    RegPoolEntry* findPhysicalByName(const char* name);
    bool func_08059464();
    bool func_080594c0();
    bool func_0805951c();
    bool func_08059578();
    bool clearVirtualRegistersForParse();
    static bool isResvd(const char* name, int cls);
    bool cycleRegisters(bool flag);
    bool clearVirtualRegisters();
    RegPoolEntry* findOrCreateRegister(const char* name);
    RegPoolEntry* findVirtualByName(const char* name);
    RegPoolEntry* createVirtualRegister(const char* name);
    bool markAllocatablePhysicalRegistersUsed();
    bool func_08059c4c();
    bool func_08059c94();
    bool func_08059cdc();
    RegPoolEntry* getMoveReg(Instr* move);
    bool ownsRegisterClass(RegPoolEntry* e);
    bool optReset();
    bool emitReset();
    char* emitStats(char* buf);
    int regIndex(int reg);
    int countUsed();
    int countMarkedPhysicalRegisters();
    int gatherInflight(ol_s** out);

    RegPoolEntry* registerEntries;
    int entryCount;
    short unk_0c;
    short unk_0e;
    RegPoolEntry* zeroRegister;   // zero register
    RegPoolEntry* externalRegister;   // ext register
    int virtualPartCount;
    RegPoolEntry* unk_1c;
    RegPoolEntry* unk_20;
    int unk_24;
    VclPtrWord unk_28;
    const char* poolName;     // pool name
};

struct RegDesc {
    const char* names[4];   // aliases; names[0] == 0 ends the table
    unsigned char allocatable;
    unsigned char registerClass;
    unsigned char partCount;
    char nonBlockId;
    unsigned char unk_14;
};

extern RegDesc regDescTable[];
extern const char* regNameChars;

int isReservedRegisterName(const char* name);

#endif

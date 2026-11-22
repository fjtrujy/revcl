#ifndef REGPOOL_H
#define REGPOOL_H

#include "vcl_portability.h"

#include "mList.h"
#include "os.h"

#include "ol_s.h"

struct rPart {
    short unk_00[0x24 / 2];
    ol_s currentLocation;
    ol_s* pendingSource;
    short cycleAge;
    short issueLock;
    short unk_40;
    short unk_42;
    short unk_44;
    short unk_46;
    VclPtrWord payload;
    ol_s pendingLocations[4];

    bool flushPending();
    ol_s* acquireFreePendingLocation();
    bool cycle(bool flag);
    void resetPartTiming();
    void resetPartFlowState();
    void resetPartPayloadFirstPass();
    void resetPartPayloadSecondPass();
    rPart();
    VclPtrWord getPayload();
    bool setPayload(VclPtrWord v);
    ol_s* getPendingLocation(int i);
};

class RegPool;
class Instr;
class RegWeb;
class instStream;
class workTable;
struct RegPoolEntry {
    short minimumMoveRow;
    short maximumMoveRow;
    RegPool *ownerPool;    // owning pool
    RegPoolEntry* next;
    const char* registerName();
    RegPoolEntry();
    void resetRegisterFlowState();
    void resetRegisterPartTiming();
    void resetRegisterThroughputState();
    mNode* addRegisterNameAlias(const char* name);
    void clearRegisterPartPayloads();
    bool cycleRegisterParts(bool flag);
    bool clearPartPayloadMarkers();
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
    void noopRegisterFlowHook();
    VclPtrWord getPartPayload(int i);
    bool setPartPayload(int i, VclPtrWord v);
    void moveReset();
    rPart* getRP(int i)
    {
        if (i < 0 || i >= partCount) {
            reportError("getRP - asked to get out of range part %s.%d / %d\n",
                          registerName(), i, partCount);
            exitAfterCleanup(-1);
        }
        return &parts[i];
    }

    RegPoolEntry* previous;
    int uniqueNameSerial;
    bool isVirtual;        // virtual
    bool allocatable;        // allocable
    bool unk_16;
    char unk_17;
    RegPoolEntry* assignedPhysicalRegister;   // allocated physical register
    char nonBlockId;
    char unk_1d[3];
    Instr* moveInstruction;
    int registerClass;         // register class
    int partCount;         // slot count (nparts)
    bool unk_2c;
    char unk_2d[3];
    int registerIndex;         // register index
    char unk_34;
    unsigned char usedSlotMask;   // used slot mask
    short unk_36;
    Instr *throughputWriter;          // previous thruput writer
    int assignmentCostTableSize;
    RegPoolEntry *nextPendingAssignment;
    rPart parts[4];
    RegWeb *registerWeb;    // register web
    mList nameAliases;

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
    bool resetPoolThroughputState();
    bool resetPoolPartTiming();
    bool resetPoolFlowState();
    bool clearPoolPartPayloads();
    bool clearVirtualRegistersForParse();
    static bool isResvd(const char* name, int cls);
    bool cycleRegisters(bool flag);
    bool clearVirtualRegisters();
    RegPoolEntry* findOrCreateRegister(const char* name);
    RegPoolEntry* findVirtualByName(const char* name);
    RegPoolEntry* createVirtualRegister(const char* name);
    bool markAllocatablePhysicalRegistersUsed();
    bool clearRegisterAssignmentsFirstPass();
    bool clearRegisterAssignmentsSecondPass();
    bool resetPoolLocalAllocationState();
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
    short registerClass;
    short unk_0e;
    RegPoolEntry* zeroRegister;   // zero register
    RegPoolEntry* externalRegister;   // ext register
    int virtualPartCount;
    RegPoolEntry* virtualHead;
    RegPoolEntry* virtualTail;
    int virtualCount;
    VclPtrWord registerPrefix;
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

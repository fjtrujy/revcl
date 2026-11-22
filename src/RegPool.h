#ifndef REGPOOL_H
#define REGPOOL_H

#include "vcl_portability.h"

#include "mList.h"
#include "os.h"

#include "ol_s.h"

struct RegisterPartState {
    short emulationTimingMetrics[0x24 / 2];
    OperandLocation currentLocation;
    OperandLocation* pendingSource;
    short cycleAge;
    short issueLock;
    short resetOnlyRegisterPartTimingSentinel;
    short earliestPendingReadCycle;
    short latestPendingReadCycle;
    short reservedRegisterPartTimingShort;
    VclPtrWord payload;
    OperandLocation pendingLocations[4];

    bool flushPending();
    OperandLocation* acquireFreePendingLocation();
    bool cycle(bool flag);
    void resetPartTiming();
    void resetPartFlowState();
    void resetPartPayloadFirstPass();
    void resetPartPayloadSecondPass();
    RegisterPartState();
    VclPtrWord getPayload();
    bool setPayload(VclPtrWord v);
    OperandLocation* getPendingLocation(int i);
};

class RegPool;
class Instr;
class RegWeb;
class InstructionStream;
class ScheduleTable;
struct RegisterEntry {
    short minimumMoveRow;
    short maximumMoveRow;
    RegPool *ownerPool;    // owning pool
    RegisterEntry* next;
    const char* registerName();
    RegisterEntry();
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
    int gatherInflight(OperandLocation** out);
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
    RegisterPartState* getRegisterPartByIndex(int i)
    {
        if (i < 0 || i >= partCount) {
            reportError("getRP - asked to get out of range part %s.%d / %d\n",
                          registerName(), i, partCount);
            exitAfterCleanup(-1);
        }
        return &parts[i];
    }

    RegisterEntry* previous;
    int uniqueNameSerial;
    bool isVirtual;        // virtual
    bool allocatable;        // allocable
    bool enabledPhysicalRegister;
    char reservedRegisterAllocationByte;
    RegisterEntry* assignedPhysicalRegister;   // allocated physical register
    char nonBlockId;
    char reservedRegisterBindingBytes[3];
    Instr* moveInstruction;
    int registerClass;         // register class
    int partCount;         // slot count (nparts)
    bool tracksHotOutputPaths;
    char reservedRegisterPartBytes[3];
    int registerIndex;         // register index
    char seenInputComponentMask;
    unsigned char usedSlotMask;   // used slot mask
    short outputWriteCount;
    Instr *throughputWriter;          // previous thruput writer
    int assignmentCostTableSize;
    RegisterEntry *nextPendingAssignment;
    RegisterPartState parts[4];
    RegWeb *registerWeb;    // register web
    mList nameAliases;

    RegWeb *getRootRegisterWeb();
};

class RegPool {
public:
    RegPool();
    virtual ~RegPool();
    virtual bool assignExt(InstructionStream *s) = 0;                        // slot 2
    virtual bool discardDisjointExternalRegister(InstructionStream *s, RegisterEntry *reg) = 0; // slot 3
    virtual bool assignRegistersForWorkTable(ScheduleTable *t) = 0;                     // slot 4
    bool initializeRegisterPool(VclPtrWord registerPrefix, const char* name, int cls, int virtualRegisterParts);
    void Dump();
    RegisterEntry* findByRegisterIndex(int index);
    RegisterEntry* findPhysicalByName(const char* name);
    bool resetPoolThroughputState();
    bool resetPoolPartTiming();
    bool resetPoolFlowState();
    bool clearPoolPartPayloads();
    bool clearVirtualRegistersForParse();
    static bool isReservedNameForClass(const char* name, int cls);
    bool cycleRegisters(bool flag);
    bool clearVirtualRegisters();
    RegisterEntry* findOrCreateRegister(const char* name);
    RegisterEntry* findVirtualByName(const char* name);
    RegisterEntry* createVirtualRegister(const char* name);
    bool markAllocatablePhysicalRegistersUsed();
    bool clearRegisterAssignmentsFirstPass();
    bool clearRegisterAssignmentsSecondPass();
    bool resetPoolLocalAllocationState();
    RegisterEntry* findMoveRegister(Instr* move);
    bool ownsRegisterClass(RegisterEntry* e);
    bool optReset();
    bool emitReset();
    char* emitStats(char* outputBuffer);
    int regIndex(int reg);
    int countUsedRegisters();
    int countMarkedPhysicalRegisters();
    int gatherInflight(OperandLocation** out);

    RegisterEntry* registerEntries;
    int entryCount;
    short registerClass;
    short poolKind;
    RegisterEntry* zeroRegister;   // zero register
    RegisterEntry* externalRegister;   // ext register
    int virtualPartCount;
    RegisterEntry* virtualHead;
    RegisterEntry* virtualTail;
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
    unsigned char tracksHotOutputPaths;
};

extern RegDesc regDescTable[];
extern const char* regNameChars;

int isReservedRegisterName(const char* name);

#endif

// Processor description (CIE group 18).
#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "vcl_portability.h"

#include "mList.h"
#include "ubsList.h"

class OpcodeDescriptor;
class RegPool;
class Instr;
class LocTag;
struct OperandLocation;

struct PoolDef {            // array ends at type == 0
    int type;               // 1 gen, 2 hw, 3 mem
    const char *prefix;     // register prefix ("vf", "vi", "hw")
    const char *name;
    int cls;                // register class, index into processorRegisterPools
    int virtualRegisterParts; // 0 forbids virtual registers; otherwise parts per virtual register
    const char *zeroReg;
    const char *extReg;
};

struct CpuDef {
    const char *name;
    int instructionsPerRawLine;
    PoolDef *pools;         // strap table
    void *devtab;
    OpcodeDescriptor *iinfos;          // itable
    void *srcSpecs;
    void *dstSpecs;
};

extern int nextInstructionSerial;
extern OpcodeDescriptor *processorInstructionTable;
extern RegPool *processorRegisterPools[13];
extern VclPtrWord freeMoveRecordsByType[3];
extern void *processorDeviceTable;    // device table
extern void *processorSourceSpecs;    // srcSpecs
extern void *processorDestinationSpecs;    // dstSpecs

class Processor {
public:
    Processor();
    ~Processor();
    bool initializeProcessor(CpuDef *cpu);
    bool clearProcessorLists();
    void dumpRegisterPools();
    int gatherInflight(OperandLocation **out);
    bool clearAllRegisterAssignmentsFirstPass();
    bool clearAllRegisterAssignmentsSecondPass();
    void validateOpcodeTableCount();

    bool setUniqueName(const char *name);
    const char *getUniqueName();
    Instr *getNextInstruction(LocTag *t, int *pos);
    bool decodeLE(LocTag *t, int *pos, mList *labels, Instr **out, int *nOut);

    bool newSyntax;
    mList exportedSymbols;
    const char *globalName;
    const char *cpuName;
    mList trailingLabels;
    char *uniqueName;          // unique name (dupString), default "FLOW"
    int instructionsPerRawLine;
    char reservedProcessorDecodeByte;
    char decodeMode;
    char decodeSubMode;
    bool insideRawLoop;
    short rawLoopId;
    int mpgTargetAddress;
    int reservedProcessorDecodeWord;
    mList visibilityScopes;
    char uniqueAll;
    char savedDecodeSubMode;
    char savedDecodeMode;
    bool vuDirectiveSeen;
    char sourceBuildType;
    char preferExistingPhysicalRegisters;
    ubsList usedBeforeSet;
    stallList stalls;
};

#endif

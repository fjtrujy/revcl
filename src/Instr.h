#ifndef INSTR_H
#define INSTR_H

#include "vcl_portability.h"
#include "vcl_modes.h"
#include "vcl_limits.h"

#include "RegPool.h"
#include "mValue.h"
#include "mList.h"
#include "os.h"

class InstructionStream;
class LocTag;
class Instr;
struct OperandLocation;
struct Operand;
struct RegisterUse;
class RegWeb;

// Opcode descriptor: mnemonic, pipeline/resource constraints, hooks and operands.
struct iOpDesc {
    unsigned char operandKind;   // 0 terminates the list
    unsigned char direction;     // VclOperandDirection: READ, WRITE, READ_WRITE
    unsigned char isImplicit;
    unsigned char outputLatency;  // forwarded to output operand timing
    unsigned char externalBlockModeFlag;
    unsigned char immediateDataType;
    unsigned char patchedOutputIndexPlusOne;
};

class OpcodeDescriptor {
public:
    char *reservedOpcodeDescriptorPointer0;
    char *mnemonic;
    char *reservedOpcodeDescriptorPointer1;
    unsigned char destinationMaskMode;
    char reservedOpcodeDescriptorByte0d;
    union {
        unsigned short pipelineAndIssueMaskWord;
        struct {
            char pipelineMask;
            char reservedOpcodeDescriptorByte0f;
        };
    };
    char resourceMask;
    char reservedOpcodeDescriptorByte11;
    char minimumIssueSpacing;
    char reservedOpcodeDescriptorBytes13[5];
    unsigned char kindAndFlags;
    unsigned char kindData;
    unsigned char branchTargetOperandSelector;
    char reservedOpcodeDescriptorByte1b;
    int efuMaskType;
    bool (*decodeHook)(void *, Instr *);
    char reservedOpcodeDescriptorBytes24[0x34 - 0x24];
    char *reverseSenseMnemonic;
    char *(*emitHook)(char *buf, Instr *in, bool upper, char *mods, bool b, int zero);
    char reservedOpcodeDescriptorBytes3c[0x44 - 0x3c];
    iOpDesc operandDescriptors[12];

    unsigned char kind() { return kindAndFlags & 7; }

    unsigned char branchFlags();
    unsigned char ramType();
    int syntheticKind();
    bool isRawOnly();
    char isNop();
    void scanOpcodeOperandDescriptors();
    void noopOpcodeDescriptorHook();
    bool check();
    int branchFlagBit1();
    int branchFlagBit3();
    int branchFlagBit2();
    int canPruneOperands();
};

// Operand-location list: list ownership and count checked by its dump helpers.
class olList {
public:
    OperandLocation *head;
    int count;

    olList()
    {
        head = 0;
        count = 0;
    }
    ~olList() { clear(); }
    bool clear();
    void validateOperandLocations();
    void dumpOperandLocations();
    OperandLocation *addOperandLocation(Instr *instruction, int op, int slot, bool ext);
    OperandLocation *addInstructionLocation(Instr *instruction);
    OperandLocation *copyOperandLocation(OperandLocation *ol, bool ext);
    OperandLocation *firstInternalLocation();
    OperandLocation *firstExternalLocation();
    OperandLocation *insertExternalLocation(OperandLocation *src);
};

// Instruction position and operand/register-part lock types retain their
// recovered data layouts and directly use their semantic C++ type names.
class RegisterPartLockList;
class InstructionPosition;
class OperandLock;

class OperandLock {
public:
    unsigned char lowerPipeInputDemand;
    unsigned char upperPipeInputDemand;
    unsigned char eitherPipeInputDemand;
    RegisterPartLockList *ownerLockList;
    OperandLock *nextSibling;
    OperandLock *parentOrChildren;
    OperandLock *nextSortedOutput;
    unsigned char lockDirection;
    char latencyModeFlag;
    char externalOutputSlotPlusOne;
    char throughputSpacing;
    char issueLatency;
    char operandStatusCode;
    char priority;
    char reservedOperandLockStatusByte;
    OperandLock *pairedLock;
    short referenceCount;
    short activeReferenceCount;
    void *ownerPointer;
    unsigned char ownerComponent;
    unsigned char ownerOperandIndex;
    short resetOnlyLockOwnerMetadata;
    char insertedIntoLockList;
    unsigned char ownerType;
    short scheduledConsumerCount;
    short loopRow;
    short linearRow;
    short adjustedEarliestReadyCycle;
    short earliestReadyCycle;
    short adjustedFullLatencyReadyCycle;
    short fullLatencyReadyCycle;
    short adjustedEarliestConsumerFreeCycle;
    short earliestConsumerFreeCycle;
    short adjustedLatestConsumerCycle;
    short latestConsumerCycle;
    short reservedLockTimingHalfword;
    short eitherPipeConsumerCount;
    short upperPipeConsumerCount;
    short lowerPipeConsumerCount;
    short adjustedLatestReadyCycle;
    short latestReadyCycle;

    OperandLock() { resetLock(); }
    ~OperandLock() {}
    bool resetLock();
    bool setOwner(int part, Instr *instr, int opnd);
    bool setOwner(int part, RegisterUse *rf);
    bool clearOwner();
    Instr *ownerInstr();
    RegisterUse *ownerRegUse();
    Operand *ownerOperand();
    int ownerOpnd();
    int getPart();
    void printOwner();
    OperandLock *getParent();
    bool setInOut(OperandLock *olk);
    bool incRef();
    bool decRef();
    char getPriority() { return priority; }
    short getRefCount() { return referenceCount; }
    void print();
    bool isOk(int type, bool inserted);
    short maximumLockConstraint();
    short secondaryLockConstraint();
    bool calcStats(Instr *skip);
    void resetCalculatedStats();
    void setFrom(InstructionPosition *r);
    short getCurrentLoopIteration();
    const char *regName();
    void resetStats()
    {
        loopRow = linearRow = -1;
        adjustedEarliestReadyCycle = -1;
        earliestReadyCycle = -1;
        adjustedFullLatencyReadyCycle = -1;
        fullLatencyReadyCycle = -1;
        adjustedEarliestConsumerFreeCycle = -1;
        earliestConsumerFreeCycle = -1;
        adjustedLatestConsumerCycle = -1;
        latestConsumerCycle = -1;
        adjustedLatestReadyCycle = latestReadyCycle = -1;
    }
};

class RegisterPartLockList {
public:
    short loopWrapMinimumRow;
    unsigned char componentIndex;
    unsigned char assigned;
    RegisterEntry *registerEntry;
    RegisterPartLockList *nextLockList;
    OperandLock *externalOutputLocks[4];
    OperandLock *outputLocks;
    OperandLock *sortedOutputLocks;
    short outputLockCount;
    short remainingLockCount;
    char containsRegisterOutput;
    char calculateLoopStats;
    unsigned char externalOutputCount;
    char sharedExternalOutputFlag;

    RegisterPartLockList();
    RegisterPartLockList(RegisterPartLockList *next);
    ~RegisterPartLockList();
    void checkCounts();
    bool reset();
    bool addInput(OperandLock *outputLock, OperandLock *in, char a, unsigned char b);
    bool addOutput(OperandLock *lk, int a, int b, unsigned char c);
    bool removeInput(OperandLock *in);
    bool removeOutput(OperandLock *lk);
    bool addOutputLock(OperandLock *lk);
    bool addInputLock(OperandLock *in);
    bool undoInputLock(OperandLock *in);
    bool undoOutputLock(OperandLock *lk);
    int maxConstraintOfFirstOutputs(int n);
    bool isOk(OperandLock *lk, int type, bool inserted);
    bool moveOutputLock(OperandLock *lk);
    bool moveInputLock(OperandLock *in);
    bool addExtOutput(OperandLock *lk, bool flag);
    Instr *earliestExt();
    int countFlaggedOutputLocks();
    bool setRepReg(RegisterEntry *selectedRegister);
    int checkInputLock(OperandLock *in, short *pos, int *minPos);
    int checkOutputLock(OperandLock *outputLock, short *pos, int *minPos);
    int checkOverlap(OperandLock *outputLock, short *pos, int *minPos);
    bool catLocks(OperandLock *a, OperandLock *b, int *s, int *e, int *end);
    int canMoveInputLock(OperandLock *in, short *pos, int slotBit);
    int canMoveOutputLock(OperandLock *outputLock, short *pos, int slotBit);
};

bool freeLockLists(RegisterPartLockList *p);
bool lockRangesOverlap(int s1, int e1, int s2, int e2);
RegisterPartLockList *findLockList(RegisterPartLockList *p, RegisterEntry *reg, int part);
const char *rlmtName(unsigned char t);

bool checkTable(OpcodeDescriptor *tab, int count);
int decodeImmediateCategory(unsigned char);
int defaultImmediateFlags(unsigned char);
unsigned char immediateFlagBit7(unsigned char);

struct OpSlot : public OperandLock {
    short conflictingOutputLockCount;
    char outputConflictMarked[2];
    olList operandLocations;

    OpSlot();
    ~OpSlot();
    void noopOperandSlotHook();
    bool resetSlotLock();
    int clearSlotLocations();
};

struct Operand {
    RegWeb *registerWeb;
    VclOperandKindByte operandKind;
    char immediateDataType;
    char externalComponentMask;
    unsigned char componentMask;
    char isImplicit;
    VclOperandDirectionByte direction;
    char outputLatency;
    char externalBlockModeFlag;
    char addressComponentMask;
    char pairedOperandIndexPlusOne;
    char reservedOperandHeaderBytes[2];
    RegisterEntry *forcedBaseRegister;
    RegisterEntry *originalRegister;
    RegisterEntry *assignedRegister;
    TypedValue immediateValue;
    char sourceTokenStartIndex;
    char sourceTokenEndIndex;
    char operandIndex;
    unsigned char externalPartMask;
    char reductionHazardMask;
    char additionalLatency;
    unsigned char pruneMask;
    char operandStatus;
    OpSlot parts[4];

    Operand();
    ~Operand();
    unsigned char getStatus() { return operandStatus; }
    bool validateKindAndMask();
    void diagnoseOperand(int);
    char *emitOperandText(char *outputBuffer, bool, bool symbolic);
    bool resetFlowState();
    bool resetOptimizationSlots();
    Instr *getOwner();
    OpSlot *getPart(int part);
    char *emitExpectedOperandType(bool flag, char *outputBuffer);
    int getRegFrom(Operand *sourceOperand);
    const char *getRegisterNameOrNone();
    RegWeb *getRootRegisterWeb();
    bool isForwardable()
    {
        return assignedRegister && assignedRegister->ownerPool->virtualPartCount && assignedRegister != assignedRegister->ownerPool->zeroRegister;
    }
    bool isZero()
    {
        int z = 0;
        if (assignedRegister) {
            if (assignedRegister == assignedRegister->ownerPool->zeroRegister)
                z = 1;
        }
        return z;
    }
    bool isReg()
    {
        bool r = false;
        if (direction == VCL_OPERAND_READ && !isZero())
            r = true;
        return r;
    }
    RegisterEntry *findPhysicalRegister() { return isReg() ? assignedRegister : 0; }
    bool isZeroReg()
    {
        bool z = false;
        if (assignedRegister) {
            if (assignedRegister == assignedRegister->ownerPool->zeroRegister)
                z = true;
        }
        return z;
    }
    RegisterEntry *inputReg()
    {
        int r = 0;
        if (direction == VCL_OPERAND_READ && !isZeroReg())
            r = 1;
        return r ? assignedRegister : 0;
    }
    RegisterEntry *inputRegister()
    {
        int r = 0;
        if (direction == VCL_OPERAND_READ && !isZeroReg2())
            r = 1;
        return (char)r ? assignedRegister : 0;
    }
    RegisterEntry *outputRegister()
    {
        int r = 0;
        if (direction == VCL_OPERAND_WRITE && !isZero())
            r = 1;
        return (char)r ? assignedRegister : 0;
    }
    RegisterEntry *outputReg()
    {
        int r = 0;
        if (direction == VCL_OPERAND_WRITE && !isZero())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegisterEntry *inputRegB()
    {
        int r = 0;
        if (direction == VCL_OPERAND_READ && !isZero())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegisterEntry *outputRegZ()
    {
        int r = 0;
        if (direction == VCL_OPERAND_WRITE) {
            if (!isZero())
                r = 1;
        }
        return r ? assignedRegister : 0;
    }
    RegisterEntry *outputRegZB()
    {
        int r = 0;
        if (direction == VCL_OPERAND_WRITE && !isZeroReg())
            r = 1;
        return r ? assignedRegister : 0;
    }
    bool isRegOp()
    {
        int r = 0;
        if (direction == VCL_OPERAND_READ && !isZero())
            r = 1;
        return r;
    }
    RegisterEntry *regOp() { return isRegOp() ? assignedRegister : 0; }
    bool isInReg() { return direction == VCL_OPERAND_READ && !isZero(); }
    bool isOutReg() { return direction == VCL_OPERAND_WRITE && !isZero(); }
    bool isReg2()
    {
        bool r = false;
        if (direction == VCL_OPERAND_WRITE && !isZero())
            r = true;
        return r;
    }
    char isOutRegOp()
    {
        int r = 0;
        if (direction == VCL_OPERAND_WRITE && !isZero())
            r = 1;
        return r;
    }
    bool isInRegOp()
    {
        int r = 0;
        if (direction == VCL_OPERAND_READ && !isZeroReg())
            r = 1;
        return r;
    }
    bool isInRegOpB() { return direction == VCL_OPERAND_READ && !isZeroReg(); }
    bool isInRegB()
    {
        bool r = false;
        if (direction == VCL_OPERAND_READ && !isZeroReg())
            r = true;
        return r;
    }
    RegisterEntry *outRegZ()
    {
        bool r = false;
        if (direction == VCL_OPERAND_WRITE && !isZeroReg2())
            r = true;
        if (r)
            return assignedRegister;
        return 0;
    }
    RegisterEntry *getReg2()
    {
        if (isReg2())
            return assignedRegister;
        return 0;
    }
    RegisterEntry *inRegOp()
    {
        bool r = false;
        if (direction == VCL_OPERAND_READ && !isZeroReg())
            r = true;
        return r ? assignedRegister : 0;
    }
    bool isZeroReg2()
    {
        bool z = false;
        if (assignedRegister) {
            int delta = (char *)assignedRegister - (char *)assignedRegister->ownerPool->zeroRegister;
            if (!delta)
                z = true;
        }
        return z;
    }
    RegisterEntry *inRegOp2()
    {
        bool r = false;
        if (direction == VCL_OPERAND_READ && !isZeroReg2())
            r = true;
        return r ? assignedRegister : 0;
    }
    RegisterEntry *outRegOp()
    {
        bool r = false;
        if (direction == VCL_OPERAND_WRITE && !isZeroReg())
            r = true;
        return r ? assignedRegister : 0;
    }
    bool isZeroB()
    {
        int z = 0;
        if (assignedRegister && assignedRegister == assignedRegister->ownerPool->zeroRegister)
            z = 1;
        return z;
    }
    RegisterEntry *outputRegB()
    {
        int r = 0;
        if (direction == VCL_OPERAND_WRITE && !isZeroB())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegisterEntry *inputRegC()
    {
        int r = 0;
        if (direction == VCL_OPERAND_READ && !isZeroB())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegisterEntry *getRegD()
    {
        bool r = false;
        if (direction == VCL_OPERAND_READ && isZeroC() == false)
            r = true;
        return r ? assignedRegister : 0;
    }
    bool isZeroC() { return assignedRegister && assignedRegister == assignedRegister->ownerPool->zeroRegister; }
    bool isInRegC() { return direction == VCL_OPERAND_READ && !isZeroC(); }
    bool isOutRegC() { return direction == VCL_OPERAND_WRITE && !isZeroC(); }
    bool isHardZero()
    {
        if (assignedRegister && assignedRegister == assignedRegister->ownerPool->zeroRegister)
            return true;
        return false;
    }
    bool isOutputReg()
    {
        int r = 0;
        if (direction == VCL_OPERAND_WRITE && !isHardZero())
            r = 1;
        return (bool)r;
    }
    bool isInputReg()
    {
        int r = 0;
        if (direction == VCL_OPERAND_READ && !isHardZero())
            r = 1;
        return (bool)r;
    }
    bool hasRegister() { return assignedRegister != 0; }
    bool isInput() { return direction == VCL_OPERAND_READ; }
    bool isOutput() { return direction == VCL_OPERAND_WRITE; }
    RegisterEntry *inReg()
    {
        if (isInput() && assignedRegister)
            return assignedRegister;
        return 0;
    }
    RegisterEntry *outReg()
    {
        if (direction == VCL_OPERAND_WRITE)
            return assignedRegister;
        return 0;
    }
};

class Instr;

static inline bool isZeroRegister(RegisterEntry *reg)
{
    return reg == reg->ownerPool->zeroRegister;
}

static inline bool isZeroOperand(Operand *op)
{
    return op->assignedRegister && isZeroRegister(op->assignedRegister);
}

static inline bool isRegisterOperand(Operand *op, char direction)
{
    return op->direction == direction && !isZeroOperand(op);
}

static inline RegisterEntry *registerOperand(Operand *op, char direction)
{
    if (isRegisterOperand(op, direction))
        return op->assignedRegister;
    return 0;
}

// Doubly linked list link embedded in Instr.
struct iLink {
    Instr *nextInstruction;
    Instr *previousInstruction;

    Instr *prev() { return previousInstruction; }
    Instr *next() { return nextInstruction; }

    iLink()
    {
        previousInstruction = 0;
        nextInstruction = 0;
    }
    ~iLink()
    {
        previousInstruction = 0;
        nextInstruction = 0;
    }
};

#include "iGroupList.h"

class InstructionPosition {
public:
    short linearRow;
    char pipelineIndex;
    char loopIteration;
    short loopRow;
    char reservedPositionTailBytes[2];

    InstructionPosition();
    ~InstructionPosition();
    void clearInstructionPosition();
    void noopInstructionPositionHook();
    void setFromLinearRow(short v);
    void setInstructionPipe(char c);
    short getRow() { return linearRow; }
    short getPipe() { return pipelineIndex; }
    void setRow(short r)
    {
        loopIteration = 0;
        linearRow = loopRow = r;
        pipelineIndex = 1;
    }
};

extern int activeLoopSize;  // range lsz
extern bool fatalReadBeforeReady; // rfUsed before result ready is fatal (name guessed)
extern bool allowInputLockLoopWrapping;
extern int activeSchedulingRangeStart;  // range start
extern int activeSchedulingRangeEnd;  // range end

class SchedulingStatistics {
public:
    short reverseLatencyDepth, reverseDependencyHeight, forwardLatencyDepth, forwardDependencyHeight;
    int moveCount, moveDistance, moveChainLength, independentLatencyDepth;

    SchedulingStatistics();
    ~SchedulingStatistics();
    void noopStatisticsHook();
    void invalidateAllCounters();
    void zeroAllCounters();
    void copyCountersFrom(SchedulingStatistics *src);
    void mergeMaximumCounters(SchedulingStatistics *o);
};

// Partial view of an instruction's owning schedule table. Only offsets used
// by the recovered routines are described; intervening bytes are uninterpreted.
struct InstructionOwnerTableView {
    char uninterpretedOwnerPrefix[9];
    char codeModeValue;
    char uninterpretedOwnerBytesBeforeStream[6];
    InstructionStream *ownerStream;

    Instr **getIE_ck(int rl);
};

class Instr {
public:
    VclPtrWord optimizedInstructionImageWord;
    VclPtrWord originalInstructionImageWord;
    Instr *cloneImage;
    short instructionId;
    union {
        unsigned char dependencyReachabilityFlags;
        struct {
            unsigned char outputDependencyReachable : 1;
            unsigned char inputDependencyReachable : 1;
        };
    };
    char reservedInstructionHeaderByte;
    InstructionGroupList instructionGroups;
    LocTag *sourceTag;
    short sourceTokenStartIndex;
    short sourceTokenEndIndex;
    iLink linearLink;
    iLink pendingLink;
    Instr *nextMoveInstruction;
    InstructionOwnerTableView *ownerTable;
    OpcodeDescriptor *opcodeInfo;  // opcode descriptor
    Operand operands[12];
    unsigned char branchFlags;     // flags
    char instructionModifierBits;
    bool rawOnly;
    unsigned char operandCount;     // operand count
    char permittedPipelineMask;
    unsigned char opcodeResourceMask;
    char schedulerCategory;
    unsigned char destinationMask;
    char reservedSchedulerFlagByte;
    char reverseBranchSense;
    char tildePrefix;
    short scheduledCycle;
    short insertionSerial;
    int replacedInstructionWord;
    char *branchLabelTags;
    olList predecessorDependencies;
    olList successorDependencies;
    short externalInputPairedConstraint;
    short externalInputConstraint;
    short externalOutputPairedConstraint;
    short externalOutputConstraint;
    mList reservedInstructionMetadataList;
    char pruningRestricted;
    char syntheticKind;
    char ramAccessType;
    unsigned char baseIssueSpacing;
    unsigned char opcodeFlags;
    char requiresFullLoopMoveDrag;
    short expectedAliasLoopId;
    char addressAliasMode;
    char addressAliasDirection;
    char addressAliasOperandIndex;
    char originalPipelineMask;
    TypedValue addressImmediateValue;
    int instructionSerial;
    int linearOrderIndex;
    InstructionPosition position;
    void clearSlot() { position.clearInstructionPosition(); }
    SchedulingStatistics schedulingStatistics;
    short dynamicUpperBound, dynamicEarliestStart, dynamicReverseStart, dynamicReverseEnd;
    short hasExtraPredecessors;     // fdep "has extra pre deps" flag
    short specialRegisterReadMask;     // slot mask read of reg idx 48
    short specialRegisterWriteMask;     // slot mask written of reg idx 48
    char reservedInstructionTimingBytes[2];
    short candidateAttemptCount;
    short candidateAttemptLimit;
    short candidateCostOffset;
    short candidateBaseRow;
    short parsedLoopId;
    char reservedInstructionMoveBytes[6];
    VclPtrWord moveRecordWord;
    short abstractMoveOpcode;
    short concreteMoveOpcode;
    short moveRowOffset;
    short moveChainDepth;
    Instr *previousThroughputWriter;        // prevThruput writer

    bool isConcreteMove()
    {
        return abstractMoveOpcode && concreteMoveOpcode;
    }
    bool isAbstractMove()
    {
        return abstractMoveOpcode && !concreteMoveOpcode;
    }
    Instr(OpcodeDescriptor *info);
    ~Instr();
    void hasImmValue();
    bool hasImmFixup();
    void resetImmFixup();
    void resetDynamicSchedulingStats();
    bool isValid()
    {
        if (!opcodeInfo)
            return false;
        for (int i = 0; i != operandCount; i++)
            if (!operands[i].validateKindAndMask())
                return false;
        return true;
    }
    bool clearDynamicData();
    bool validateOperands();
    void dumpOperandDiagnostics(int flags);
    bool isNop();
    InstructionStream *getOwningStream();  // owner instruction stream
    bool resetFlowDependencies();
    unsigned char getBranchType() { return branchFlags; }
    bool isNonSynthetic();
    bool clearTransientFlags();
    void dumpInstructionContext();
    void dumpInstructionContext(int opnd);
    void dumpInstructionContext(int opnd, int mask);
    bool patchBranchLatencies();
    int simplifyBranch();
    int normalizeZeroSources();
    int opcodeId();
    Operand *getOp(int n)
    {
        if (n < 0) {
            reportError("-ve operand passed to getOp!\n");
            exitAfterCleanup(0);
            return 0;
        }
        return n < operandCount ? &operands[n] : 0;
    }
    Instr *clone();
    int getRamBits();
    bool resetOptimizationState();
    int collectOutputRegisterMask(RegisterEntry *reg, bool *paired);
    int collectInputRegisterMask(RegisterEntry *reg, bool *paired);
    bool getImmediateOperand(int n, int *out);
    bool changeBranchImmediate(const char *s);
    bool setOwner(InstructionOwnerTableView *s);
    char *emitExpectedSyntax(bool flag, char *outputBuffer, int n);
    int outputMaskForRegister(RegisterEntry *registerEntry, int mask);
    int inputMaskForRegister(RegisterEntry *registerEntry, int mask);
    bool hasRawTableOrImmediateToken();
    char *getBranchTargetImmediate();
    RegisterEntry *getBranchTargetRegister();
    int constantZeroInstructionHook();
    Operand *findInputOperand(RegisterEntry *reg, int mask);
    Operand *findOutputOperand(RegisterEntry *reg, int mask);
    bool hasLinkedOutputFrom(unsigned int n);
    bool classifySchedulingCategory();
    Instr **getIE();
    int getFirstOutputLatency();
    bool markDependencyInputReachable();
    bool markDependencyOutputReachable();
    bool expandVirtualStatOperand();
    void clearExternalTimingConstraints();
    void copyExternalTimingConstraints(Instr *sourceInstruction);
    void raiseExternalInputPairedConstraint(short v);
    void raiseExternalInputConstraint(short v);
    void raiseExternalOutputPairedConstraint(short v);
    void raiseExternalOutputConstraint(short v);
};

OpcodeDescriptor *getOpcodeInfoById(int id);
OpcodeDescriptor *findOpcodeInfoByMnemonic(const char *name, bool);
RegisterEntry *findRegisterByGlobalIndex(int idx);
int getOpcodeId(OpcodeDescriptor *op);
Instr *getInstructionInPipe(Instr **ie, int p);
int checkDispatchEntry(Instr **ie);
bool wouldPair(Instr **ie, Instr *in, unsigned p);
bool isVuRegister(RegisterEntry *r);

// Address-alias descriptor per opcode (12-byte entries).
struct AddressAliasDescriptor {
    int opcodeId;
    char aliasMode;
    unsigned char inputOperandIndex;
    unsigned char outputOperandIndex;
    signed char immediateOperandIndex;
    int literalImmediate;
};

// Address-alias operand-use entry per opcode (8-byte entries).
struct AddressAliasUse {
    int opcodeId;
    unsigned char operandIndex;
};

AddressAliasDescriptor *findAddressAliasDescriptor(Instr *in);
AddressAliasUse *findAddressAliasUse(Instr *in);
RegisterEntry *getOperandZeroRegister(int kind);
bool hasDirectOperandDependency(Instr *a, Instr *b);
bool writesInputRegisterOf(Instr *a, Instr *b);
Operand *operandAtLocation(OperandLocation *ol);
void markLocationExternallyUsed(OperandLocation *ol);
int hasRawOrImmediateInstructionAtLocation(OperandLocation *ol);
int instructionSerialAtLocation(OperandLocation *ol);
bool sameOperandLocation(OperandLocation *a, OperandLocation *b);
InstructionStream *locationOwningStream(OperandLocation *ol);
int locationRow(OperandLocation *ol);
OperandLocation *allocateLocationNode();
int countFreeLocationNodes();
bool linkOperandParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot);
bool linkOperandParts(OperandLocation *o, OperandLocation *i, int x, int y);
const char *branchTypeNameByCode(int t);
const char *syntheticTypeName(int t);

// Parsed implicit operand name, physical binding, component masks and literals.
struct ImplicitOperandDescription {
    char name[VCL_IMPLICIT_OPERAND_TEXT_CAPACITY];
    char physicalRegisterName[VCL_IMPLICIT_OPERAND_TEXT_CAPACITY];
    char immediateText[VCL_IMPLICIT_OPERAND_TEXT_CAPACITY];
    char branchLabelText[VCL_IMPLICIT_OPERAND_TEXT_CAPACITY];
    int componentMask;
    int addressComponentMask;
    int genericOperandForm;
    char forcePhysicalBinding;
    char reservedImplicitOperandTail[3];
};

bool buildImplicitOperand(int kind, ImplicitOperandDescription *out, bool);
const char *opKindName(int kind);
int registerClassForOperandKind(int kind);
bool isScalarFloatingOperandKind(int kind);
bool isVectorOrAccumulatorOperandKind(int kind);
const char *immediateTypeName(unsigned char);
extern OperandLocation *olFreeList;

struct opPair_s {
    int opcode;
    signed char operandPairSlots[4];
    int pairingMode;
};

struct opCount_s {
    int opcode;
    int operandCount;
};

extern opPair_s pairedOperandDescriptions[];
extern opCount_s operandCountOverrides[];

#endif

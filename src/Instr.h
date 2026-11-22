#ifndef INSTR_H
#define INSTR_H

#include "vcl_portability.h"

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
    unsigned char operandKind;   // 0 terminates the list
    unsigned char direction;     // 1 = input, 2 = output
    unsigned char isImplicit;
    unsigned char outputLatency;  // forwarded to output operand timing
    unsigned char externalBlockModeFlag;
    unsigned char immediateDataType;
    unsigned char patchedOutputIndexPlusOne;
};

class iInfo {
public:
    char *unk_00;
    char *mnemonic;
    char *unk_08;
    unsigned char destinationMaskMode;
    char unk_0d;
    union {
        unsigned short unk_0e_w;
        struct {
            char pipelineMask;
            char unk_0f;
        };
    };
    char resourceMask;
    char unk_11;
    char minimumIssueSpacing;
    char unk_13[5];
    unsigned char kindAndFlags;
    unsigned char kindData;
    unsigned char branchTargetOperandSelector;
    char unk_1b;
    int efuMaskType;
    bool (*decodeHook)(void *, Instr *);
    char unk_24[0x34 - 0x24];
    char *reverseSenseMnemonic;
    char *(*emitHook)(char *buf, Instr *in, bool upper, char *mods, bool b, int zero);
    char unk_3c[0x44 - 0x3c];
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
    ol_s *head;
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
    ol_s *addOperandLocation(Instr *inst, int op, int slot, bool ext);
    ol_s *addInstructionLocation(Instr *inst);
    ol_s *copyOperandLocation(ol_s *ol, bool ext);
    ol_s *firstInternalLocation();
    ol_s *firstExternalLocation();
    ol_s *insertExternalLocation(ol_s *src);
};

// Base of OpSlot: fields 0x00..0x4f reset by resetLock (class name guessed).
class cls0805c244;
class cls08058718;
class cls0805bcc4;

// Descriptive source-level type aliases retain the original C++ class names
// and mangled symbols required for the byte-exact historical ELF.
typedef cls08058718 InstructionPosition;
typedef cls0805bcc4 OperandLock;
typedef cls0805c244 RegisterPartLockList;

class cls0805bcc4 {
public:
    unsigned char lowerPipeInputDemand;
    unsigned char upperPipeInputDemand;
    unsigned char eitherPipeInputDemand;
    cls0805c244 *ownerLockList;
    cls0805bcc4 *nextSibling;
    cls0805bcc4 *parentOrChildren;
    cls0805bcc4 *nextSortedOutput;
    unsigned char lockDirection;
    char latencyModeFlag;
    char externalOutputSlotPlusOne;
    char throughputSpacing;
    char issueLatency;
    char operandStatusCode;
    char priority;
    char unk_1b;
    cls0805bcc4 *pairedLock;
    short referenceCount;
    short activeReferenceCount;
    void *ownerPointer;
    unsigned char ownerComponent;
    unsigned char ownerOperandIndex;
    short unk_2a;
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
    short unk_44;
    short eitherPipeConsumerCount;
    short upperPipeConsumerCount;
    short lowerPipeConsumerCount;
    short adjustedLatestReadyCycle;
    short latestReadyCycle;

    cls0805bcc4() { resetLock(); }
    ~cls0805bcc4() {}
    bool resetLock();
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
    char getPriority() { return priority; }
    short getRefCount() { return referenceCount; }
    void print();
    bool isOk(int type, bool inserted);
    short maximumLockConstraint();
    short secondaryLockConstraint();
    bool calcStats(Instr *skip);
    void resetCalculatedStats();
    void setFrom(cls08058718 *r);
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

class cls0805c244 {
public:
    short loopWrapMinimumRow;
    unsigned char componentIndex;
    unsigned char assigned;
    RegPoolEntry *registerEntry;
    cls0805c244 *nextLockList;
    cls0805bcc4 *externalOutputLocks[4];
    cls0805bcc4 *outputLocks;
    cls0805bcc4 *sortedOutputLocks;
    short outputLockCount;
    short remainingLockCount;
    char containsRegisterOutput;
    char calculateLoopStats;
    unsigned char externalOutputCount;
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
    int maxConstraintOfFirstOutputs(int n);
    bool isOk(cls0805bcc4 *lk, int type, bool inserted);
    bool moveOutputLock(cls0805bcc4 *lk);
    bool moveInputLock(cls0805bcc4 *in);
    bool addExtOutput(cls0805bcc4 *lk, bool flag);
    Instr *earliestExt();
    int countFlaggedOutputLocks();
    bool setRepReg(RegPoolEntry *rg);
    int checkInputLock(cls0805bcc4 *in, short *pos, int *minPos);
    int checkOutputLock(cls0805bcc4 *out, short *pos, int *minPos);
    int checkOverlap(cls0805bcc4 *out, short *pos, int *minPos);
    bool catLocks(cls0805bcc4 *a, cls0805bcc4 *b, int *s, int *e, int *end);
    int canMoveInputLock(cls0805bcc4 *in, short *pos, int slotBit);
    int canMoveOutputLock(cls0805bcc4 *out, short *pos, int slotBit);
};

bool freeLockLists(cls0805c244 *p);
bool lockRangesOverlap(int s1, int e1, int s2, int e2);
cls0805c244 *findLockList(cls0805c244 *p, RegPoolEntry *reg, int part);
const char *rlmtName(unsigned char t);

bool checkTable(iInfo *tab, int count);
int decodeImmediateCategory(unsigned char);
int defaultImmediateFlags(unsigned char);
unsigned char immediateFlagBit7(unsigned char);

struct OpSlot : public cls0805bcc4 {
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
    char operandKind;
    char immediateDataType;
    char externalComponentMask;
    unsigned char componentMask;
    char isImplicit;
    char direction;
    char outputLatency;
    char externalBlockModeFlag;
    char addressComponentMask;
    char pairedOperandIndexPlusOne;
    char unk_0e[2];
    RegPoolEntry *forcedBaseRegister;
    RegPoolEntry *originalRegister;
    RegPoolEntry *assignedRegister;
    mValue immediateValue;
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
    char *Emit(char *buf, bool, bool symbolic);
    bool resetFlowState();
    bool resetOptimizationSlots();
    Instr *getOwner();
    OpSlot *getPart(int part);
    char *emitExpectedOperandType(bool flag, char *buf);
    int getRegFrom(Operand *src);
    const char *getRegisterNameOrNone();
    RegWeb *GetWeb();
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
        if (direction == 1 && !isZero())
            r = true;
        return r;
    }
    RegPoolEntry *getReg() { return isReg() ? assignedRegister : 0; }
    bool isZeroReg()
    {
        bool z = false;
        if (assignedRegister) {
            if (assignedRegister == assignedRegister->ownerPool->zeroRegister)
                z = true;
        }
        return z;
    }
    RegPoolEntry *inputReg()
    {
        int r = 0;
        if (direction == 1 && !isZeroReg())
            r = 1;
        return r ? assignedRegister : 0;
    }
    RegPoolEntry *inputRegister()
    {
        int r = 0;
        if (direction == 1 && !isZeroReg2())
            r = 1;
        return (char)r ? assignedRegister : 0;
    }
    RegPoolEntry *outputRegister()
    {
        int r = 0;
        if (direction == 2 && !isZero())
            r = 1;
        return (char)r ? assignedRegister : 0;
    }
    RegPoolEntry *outputReg()
    {
        int r = 0;
        if (direction == 2 && !isZero())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegPoolEntry *inputRegB()
    {
        int r = 0;
        if (direction == 1 && !isZero())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegPoolEntry *outputRegZ()
    {
        int r = 0;
        if (direction == 2) {
            if (!isZero())
                r = 1;
        }
        return r ? assignedRegister : 0;
    }
    RegPoolEntry *outputRegZB()
    {
        int r = 0;
        if (direction == 2 && !isZeroReg())
            r = 1;
        return r ? assignedRegister : 0;
    }
    bool isRegOp()
    {
        int r = 0;
        if (direction == 1 && !isZero())
            r = 1;
        return r;
    }
    RegPoolEntry *regOp() { return isRegOp() ? assignedRegister : 0; }
    bool isInReg() { return direction == 1 && !isZero(); }
    bool isOutReg() { return direction == 2 && !isZero(); }
    bool isReg2()
    {
        bool r = false;
        if (direction == 2 && !isZero())
            r = true;
        return r;
    }
    char isOutRegOp()
    {
        int r = 0;
        if (direction == 2 && !isZero())
            r = 1;
        return r;
    }
    bool isInRegOp()
    {
        int r = 0;
        if (direction == 1 && !isZeroReg())
            r = 1;
        return r;
    }
    bool isInRegOpB() { return direction == 1 && !isZeroReg(); }
    bool isInRegB()
    {
        bool r = false;
        if (direction == 1 && !isZeroReg())
            r = true;
        return r;
    }
    RegPoolEntry *outRegZ()
    {
        bool r = false;
        if (direction == 2 && !isZeroReg2())
            r = true;
        if (r)
            return assignedRegister;
        return 0;
    }
    RegPoolEntry *getReg2()
    {
        if (isReg2())
            return assignedRegister;
        return 0;
    }
    RegPoolEntry *inRegOp()
    {
        bool r = false;
        if (direction == 1 && !isZeroReg())
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
    RegPoolEntry *inRegOp2()
    {
        bool r = false;
        if (direction == 1 && !isZeroReg2())
            r = true;
        return r ? assignedRegister : 0;
    }
    RegPoolEntry *outRegOp()
    {
        bool r = false;
        if (direction == 2 && !isZeroReg())
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
    RegPoolEntry *outputRegB()
    {
        int r = 0;
        if (direction == 2 && !isZeroB())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegPoolEntry *inputRegC()
    {
        int r = 0;
        if (direction == 1 && !isZeroB())
            r = 1;
        return (bool)r ? assignedRegister : 0;
    }
    RegPoolEntry *getRegD()
    {
        bool r = false;
        if (direction == 1 && isZeroC() == false)
            r = true;
        return r ? assignedRegister : 0;
    }
    bool isZeroC() { return assignedRegister && assignedRegister == assignedRegister->ownerPool->zeroRegister; }
    bool isInRegC() { return direction == 1 && !isZeroC(); }
    bool isOutRegC() { return direction == 2 && !isZeroC(); }
    bool isHardZero()
    {
        if (assignedRegister && assignedRegister == assignedRegister->ownerPool->zeroRegister)
            return true;
        return false;
    }
    bool isOutputReg()
    {
        int r = 0;
        if (direction == 2 && !isHardZero())
            r = 1;
        return (bool)r;
    }
    bool isInputReg()
    {
        int r = 0;
        if (direction == 1 && !isHardZero())
            r = 1;
        return (bool)r;
    }
    bool hasRegister() { return assignedRegister != 0; }
    bool isInput() { return direction == 1; }
    bool isOutput() { return direction == 2; }
    RegPoolEntry *inReg()
    {
        if (isInput() && assignedRegister)
            return assignedRegister;
        return 0;
    }
    RegPoolEntry *outReg()
    {
        if (direction == 2)
            return assignedRegister;
        return 0;
    }
};

class Instr;

static inline bool isZeroRegister(RegPoolEntry *reg)
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

static inline RegPoolEntry *registerOperand(Operand *op, char direction)
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

class cls08058718 {
public:
    short linearRow;
    char pipelineIndex;
    char loopIteration;
    short loopRow;
    char unk_06[2];

    cls08058718();
    ~cls08058718();
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

class cls0805e2e4 {
public:
    short reverseLatencyDepth, reverseDependencyHeight, forwardLatencyDepth, forwardDependencyHeight;
    int moveCount, moveDistance, moveChainLength, independentLatencyDepth;

    cls0805e2e4();
    ~cls0805e2e4();
    void noopStatisticsHook();
    void invalidateAllCounters();
    void zeroAllCounters();
    void copyCountersFrom(cls0805e2e4 *src);
    void mergeMaximumCounters(cls0805e2e4 *o);
};

// Placeholder for the object at Instr::unk_30 (type unknown).
struct cls_unk30 {
    char unk_00[9];
    char codeModeValue;
    char unk_0a[6];
    instStream *ownerStream;

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
    char unk_0f;
    iGroupList instructionGroups;
    LocTag *sourceTag;
    short sourceTokenStartIndex;
    short sourceTokenEndIndex;
    iLink linearLink;
    iLink pendingLink;
    Instr *nextMoveInstruction;
    cls_unk30 *ownerTable;
    iInfo *opcodeInfo;  // opcode descriptor
    Operand operands[12];
    unsigned char branchFlags;     // flags
    char instructionModifierBits;
    bool rawOnly;
    unsigned char operandCount;     // operand count
    char permittedPipelineMask;
    unsigned char opcodeResourceMask;
    char schedulerCategory;
    unsigned char destinationMask;
    char unk_1390;
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
    mList unk_13b8;
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
    mValue addressImmediateValue;
    int instructionSerial;
    int linearOrderIndex;
    cls08058718 position;
    void clearSlot() { position.clearInstructionPosition(); }
    cls0805e2e4 schedulingStatistics;
    short dynamicUpperBound, dynamicEarliestStart, dynamicReverseStart, dynamicReverseEnd;
    short hasExtraPredecessors;     // fdep "has extra pre deps" flag
    short specialRegisterReadMask;     // slot mask read of reg idx 48
    short specialRegisterWriteMask;     // slot mask written of reg idx 48
    char unk_1416[2];
    short candidateAttemptCount;
    short candidateAttemptLimit;
    short candidateCostOffset;
    short candidateBaseRow;
    short parsedLoopId;
    char unk_1422[6];
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
    Instr(iInfo *info);
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
    instStream *getOwningStream();  // owner instruction stream
    bool resetFlowDependencies();
    unsigned char getBranchType() { return branchFlags; }
    bool isNonSynthetic();
    bool clearTransientFlags();
    void leDump();
    void leDump(int opnd);
    void leDump(int opnd, int mask);
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
    int collectOutputRegisterMask(RegPoolEntry *reg, bool *paired);
    int collectInputRegisterMask(RegPoolEntry *reg, bool *paired);
    bool getImmediateOperand(int n, int *out);
    bool changeBranchImmediate(const char *s);
    bool setOwner(cls_unk30 *s);
    char *emitExpectedSyntax(bool flag, char *buf, int n);
    int outputMaskForRegister(RegPoolEntry *reg, int mask);
    int inputMaskForRegister(RegPoolEntry *reg, int mask);
    bool hasRawTableOrImmediateToken();
    char *getBranchTargetImmediate();
    RegPoolEntry *getBranchTargetRegister();
    int constantZeroInstructionHook();
    Operand *findInputOperand(RegPoolEntry *reg, int mask);
    Operand *findOutputOperand(RegPoolEntry *reg, int mask);
    bool hasLinkedOutputFrom(unsigned int n);
    bool classifySchedulingCategory();
    Instr **getIE();
    int getFirstOutputLatency();
    bool markDependencyInputReachable();
    bool markDependencyOutputReachable();
    bool expandVirtualStatOperand();
    void clearExternalTimingConstraints();
    void copyExternalTimingConstraints(Instr *src);
    void raiseExternalInputPairedConstraint(short v);
    void raiseExternalInputConstraint(short v);
    void raiseExternalOutputPairedConstraint(short v);
    void raiseExternalOutputConstraint(short v);
};

iInfo *getOpcodeInfoById(int id);
iInfo *findOpcodeInfoByMnemonic(const char *name, bool);
RegPoolEntry *findRegisterByGlobalIndex(int idx);
int getOpcodeId(iInfo *op);
Instr *GetI(Instr **ie, int p);
int checkDispatchEntry(Instr **ie);
bool wouldPair(Instr **ie, Instr *in, unsigned p);
bool isVuRegister(RegPoolEntry *r);

// Address-alias descriptor per opcode (12-byte entries, names guessed).
struct aaDesc {
    int opcodeId;
    char aliasMode;
    unsigned char inputOperandIndex;
    unsigned char outputOperandIndex;
    signed char immediateOperandIndex;
    int literalImmediate;
};

// Address-alias use entry per opcode (8-byte entries, names guessed).
struct aaUse {
    int opcodeId;
    unsigned char operandIndex;
};

aaDesc *findAddressAliasDescriptor(Instr *in);
aaUse *findAddressAliasUse(Instr *in);
RegPoolEntry *getOperandZeroReg(int kind);
bool hasDirectOperandDependency(Instr *a, Instr *b);
bool writesInputRegisterOf(Instr *a, Instr *b);
Operand *operandAtLocation(ol_s *ol);
void markLocationExternallyUsed(ol_s *ol);
int hasRawOrImmediateInstructionAtLocation(ol_s *ol);
int instructionSerialAtLocation(ol_s *ol);
bool sameOperandLocation(ol_s *a, ol_s *b);
instStream *locationOwningStream(ol_s *ol);
int locationRow(ol_s *ol);
ol_s *allocateLocationNode();
int countFreeLocationNodes();
bool link_oParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot);
bool link_oParts(ol_s *o, ol_s *i, int x, int y);
const char *branchTypeNameByCode(int t);
const char *syntheticTypeName(int t);

// Implicit operand description (name guessed).
struct ImplicitOp {
    char name[0x800];
    char physicalRegisterName[0x800];
    char immediateText[0x800];
    char branchLabelText[0x800];
    int componentMask;
    int addressComponentMask;
    int genericOperandForm;
    char forcePhysicalBinding;
    char unk_200d[3];
};

bool buildImplicitOperand(int kind, ImplicitOp *out, bool);
const char *opKindName(int kind);
int registerClassForOperandKind(int kind);
bool isScalarFloatingOperandKind(int kind);
bool isVectorOrAccumulatorOperandKind(int kind);
const char *immediateTypeName(unsigned char);
extern ol_s *olFreeList;

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

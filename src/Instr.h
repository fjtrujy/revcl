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
    unsigned char unk_04;
    unsigned char unk_05;
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
            char unk_0e;
            char unk_0f;
        };
    };
    char unk_10;
    char unk_11;
    char unk_12;
    char unk_13[5];
    unsigned char kindAndFlags;
    unsigned char kindData;
    unsigned char unk_1a;   // branch target operand index + 1
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
    void func_08070080();
    void func_080700a4();
    bool check();
    int func_080701f4();
    int func_0807021c();
    int func_08070244();
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

// Base of OpSlot: fields 0x00..0x4f reset by func_0805bcc4 (class name guessed).
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
    unsigned char unk_00;
    unsigned char unk_01;
    unsigned char unk_02;
    cls0805c244 *unk_04;    // owning locklist
    cls0805bcc4 *unk_08;    // next sibling in parent's child list
    cls0805bcc4 *unk_0c;    // input: parent output lock; output: child list head
    cls0805bcc4 *unk_10;    // next in locklist sorted output list
    unsigned char unk_14;   // 1 = input lock, 2 = output lock
    char unk_15;
    char unk_16;
    char unk_17;
    char unk_18;
    char unk_19;
    char unk_1a;
    char unk_1b;
    cls0805bcc4 *unk_1c;    // in/out partner lock
    short unk_20;
    short unk_22;
    void *unk_24;           // owner: Instr* (oType 1) or regUse* (oType 2)
    unsigned char unk_28;   // part
    unsigned char unk_29;   // operand index
    short unk_2a;
    char unk_2c;
    unsigned char unk_2d;   // owner type: 0 none, 1 Instr, 2 regUse
    short unk_2e;
    short unk_30;
    short unk_32;
    short unk_34;
    short unk_36;
    short unk_38;
    short unk_3a;
    short unk_3c;
    short unk_3e;
    short unk_40;
    short unk_42;
    short unk_44;
    short unk_46;
    short unk_48;
    short unk_4a;
    short unk_4c;
    short unk_4e;

    cls0805bcc4() { func_0805bcc4(); }
    ~cls0805bcc4() {}
    bool func_0805bcc4();
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
    char getPriority() { return unk_1a; }
    short getRefCount() { return unk_20; }
    void print();
    bool isOk(int type, bool inserted);
    short func_0805ccc8();
    short func_0805ccd4();
    bool calcStats(Instr *skip);
    void func_0805d020();
    void setFrom(cls08058718 *r);
    short func_0805d0dc();
    const char *regName();
    void resetStats()
    {
        unk_30 = unk_32 = -1;
        unk_34 = -1;
        unk_36 = -1;
        unk_38 = -1;
        unk_3a = -1;
        unk_3c = -1;
        unk_3e = -1;
        unk_40 = -1;
        unk_42 = -1;
        unk_4c = unk_4e = -1;
    }
};

class cls0805c244 {
public:
    short unk_00;
    unsigned char unk_02;   // part
    unsigned char unk_03;   // assigned flag
    RegPoolEntry *unk_04;   // register
    cls0805c244 *unk_08;    // next locklist
    cls0805bcc4 *unk_0c[4]; // external output locks
    cls0805bcc4 *unk_1c;    // output lock list (linked by lock->unk_08)
    cls0805bcc4 *unk_20;    // sorted output list head (by unk_32, linked by unk_10)
    short unk_24;           // nOutLocks
    short unk_26;           // nToGo
    char unk_28;
    char unk_29;
    unsigned char unk_2a;   // extern output count
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
    int func_0805c938(int n);
    bool isOk(cls0805bcc4 *lk, int type, bool inserted);
    bool moveOutputLock(cls0805bcc4 *lk);
    bool moveInputLock(cls0805bcc4 *in);
    bool addExtOutput(cls0805bcc4 *lk, bool flag);
    Instr *earliestExt();
    int func_0805d3c4();
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
int func_08070300(unsigned char);
int func_0807031c(unsigned char);
unsigned char func_08070324(unsigned char);

struct OpSlot : public cls0805bcc4 {
    short unk_50;
    char unk_52[2];
    olList unk_54;          // uses

    OpSlot();
    ~OpSlot();
    void func_080560bc();
    bool resetSlotLock();
    int clearSlotLocations();
};

struct Operand {
    RegWeb *registerWeb;
    char operandKind;
    char unk_05;
    char externalComponentMask;
    unsigned char componentMask;
    char isImplicit;
    char direction;
    char outputLatency;
    char unk_0b;
    char unk_0c;
    char pairedOperandIndexPlusOne;
    char unk_0e[2];
    RegPoolEntry *forcedBaseRegister;
    RegPoolEntry *originalRegister;
    RegPoolEntry *assignedRegister;
    mValue immediateValue;
    char unk_24;
    char unk_25;
    char operandIndex;
    unsigned char externalPartMask;
    char unk_28;
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
    Instr *unk_00;  // next
    Instr *unk_04;  // prev

    Instr *prev() { return unk_04; }
    Instr *next() { return unk_00; }

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

#include "iGroupList.h"

class cls08058718 {
public:
    short unk_00;
    char unk_02;
    char unk_03;
    short unk_04;
    char unk_06[2];

    cls08058718();
    ~cls08058718();
    void clearInstructionPosition();
    void func_08058744();
    void setFromLinearRow(short v);
    void setInstructionPipe(char c);
    short getRow() { return unk_00; }
    short getPipe() { return unk_02; }
    void setRow(short r)
    {
        unk_03 = 0;
        unk_00 = unk_04 = r;
        unk_02 = 1;
    }
};

extern int g_080b336c;  // range lsz
extern bool g_080de284; // rfUsed before result ready is fatal (name guessed)
extern bool g_080de288;
extern int g_080b3370;  // range start
extern int g_080b3374;  // range end

class cls0805e2e4 {
public:
    short unk_00, unk_02, unk_04, unk_06;
    int unk_08, unk_0c, unk_10, unk_14;

    cls0805e2e4();
    ~cls0805e2e4();
    void func_0805e2fc();
    void invalidateAllCounters();
    void zeroAllCounters();
    void copyCountersFrom(cls0805e2e4 *src);
    void mergeMaximumCounters(cls0805e2e4 *o);
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
    VclPtrWord unk_00;
    VclPtrWord unk_04;
    Instr *unk_08;  // tmpClone
    short unk_0c;   // id
    union {
        unsigned char unk_0e;
        struct {
            unsigned char unk_0e_0 : 1;
            unsigned char unk_0e_1 : 1;
        };
    };
    char unk_0f;
    iGroupList unk_10;
    LocTag *unk_14;
    short unk_18;
    short unk_1a;
    iLink unk_1c;   // linear list link
    iLink unk_24;   // ToDo/Done list link
    Instr *unk_2c;  // next in Block move list
    cls_unk30 *unk_30;
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
    int unk_1398;
    char *branchLabelTags;
    olList predecessorDependencies;
    olList successorDependencies;
    short unk_13b0;
    short unk_13b2;
    short unk_13b4;
    short unk_13b6;
    mList unk_13b8;
    char unk_13cc;
    char syntheticKind;
    char ramAccessType;
    unsigned char baseIssueSpacing;
    unsigned char opcodeFlags;
    char unk_13d1;
    short expectedAliasLoopId;
    char addressAliasMode;
    char addressAliasDirection;
    char addressAliasOperandIndex;
    char unk_13d7;
    mValue addressImmediateValue;
    int instructionSerial;
    int linearOrderIndex;
    cls08058718 position;
    void clearSlot() { position.clearInstructionPosition(); }
    cls0805e2e4 unk_13f0;
    short dynamicUpperBound, dynamicEarliestStart, dynamicReverseStart, dynamicReverseEnd;
    short hasExtraPredecessors;     // fdep "has extra pre deps" flag
    short specialRegisterReadMask;     // slot mask read of reg idx 48
    short specialRegisterWriteMask;     // slot mask written of reg idx 48
    char unk_1416[2];
    short unk_1418;
    short unk_141a;
    short unk_141c;
    short unk_141e;
    short parsedLoopId;
    char unk_1422[6];
    VclPtrWord unk_1428;
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
    void func_08054008(int flags);
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
    int func_08055ac4();
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
    char *func_080552a8(bool flag, char *buf, int n);
    int outputMaskForRegister(RegPoolEntry *reg, int mask);
    int inputMaskForRegister(RegPoolEntry *reg, int mask);
    bool func_08055548();
    char *getBranchTargetImmediate();
    RegPoolEntry *getBranchTargetRegister();
    int func_08055718();
    Operand *findInputOperand(RegPoolEntry *reg, int mask);
    Operand *findOutputOperand(RegPoolEntry *reg, int mask);
    bool hasLinkedOutputFrom(unsigned int n);
    bool func_080558f4();
    Instr **getIE();
    int getFirstOutputLatency();
    bool func_08055a88();
    bool func_08055aa8();
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
    int unk_00;             // opcode id
    char unk_04;            // -> Instr::addressAliasMode
    unsigned char unk_05;   // operand index
    unsigned char unk_06;   // output operand index
    signed char unk_07;     // immediate operand index, <0 => unk_08
    int unk_08;             // literal immediate
};

// Address-alias use entry per opcode (8-byte entries, names guessed).
struct aaUse {
    int unk_00;             // opcode id
    unsigned char unk_04;   // operand index
};

aaDesc *findAddressAliasDescriptor(Instr *in);
aaUse *findAddressAliasUse(Instr *in);
RegPoolEntry *getOperandZeroReg(int kind);
bool func_080543bc(Instr *a, Instr *b);
bool func_0805449c(Instr *a, Instr *b);
Operand *operandAtLocation(ol_s *ol);
void markLocationExternallyUsed(ol_s *ol);
int func_080585a8(ol_s *ol);
int instructionSerialAtLocation(ol_s *ol);
bool sameOperandLocation(ol_s *a, ol_s *b);
instStream *locationOwningStream(ol_s *ol);
int locationRow(ol_s *ol);
ol_s *allocateLocationNode();
int countFreeLocationNodes();
bool link_oParts(Instr *a, int aOp, int slot, Instr *b, int bOp, int bSlot);
bool link_oParts(ol_s *o, ol_s *i, int x, int y);
const char *func_08055254(int t);
const char *func_080554e4(int t);

// Implicit operand description (name guessed).
struct ImplicitOp {
    char name[0x800];
    char unk_800[0x800];
    char unk_1000[0x800];
    char unk_1800[0x800];
    int unk_2000;           // slot mask
    int unk_2004;
    int unk_2008;
    char unk_200c;
    char unk_200d[3];
};

bool buildImplicitOperand(int kind, ImplicitOp *out, bool);
const char *opKindName(int kind);
int func_08057a34(int kind);
bool func_08057d60(int kind);
bool func_08057d9c(int kind);
const char *func_0806a33c(unsigned char);
extern ol_s *olFreeList;

struct opPair_s {
    int opcode;
    signed char unk_04[4];
    int unk_08;
};

struct opCount_s {
    int opcode;
    int unk_04;
};

extern opPair_s opPairTable_080b32e0[];
extern opCount_s opCountTable_080b3340[];

#endif

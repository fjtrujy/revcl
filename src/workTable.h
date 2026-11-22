// Work table: per-register use lists (names guessed), CIE group 6 owns the lookup.
#ifndef WORKTABLE_H
#define WORKTABLE_H

#include "os.h"
#include "Instr.h"
#include "tPairList.h"
#include "regUseSlot.h"
#include "wtEntry.h"

struct OperandLocation;
class InstructionStream;
struct RegisterEntry;

struct RegisterUse {
    RegisterUse *previous;
    RegisterUse *next;
    RegisterEntry *registerEntry;
    unsigned char externalInputMask;
    unsigned char externalOutputMask;
    unsigned char inputMask;
    unsigned char outputMask;
    unsigned char throughputMask;
    bool copiedRegisterUseFlag;
    unsigned char hazardMask;
    unsigned char sourcePathHazardMask;
    unsigned char hazardBroadcastMask;
    char externalBlockMode;
    short copiedRegisterUseMetadata;
    short recordedOutputWriteCount;
    Instr *definingInstruction;
    regUseSlot slots[4];

    RegisterUse(RegisterEntry *reg);
    ~RegisterUse();
    void validateAndDumpParts();
    RegisterUse *nextReal();
    RegisterUse *nextVirtual();
    bool optReset();
    RegisterUse *Clone();
    const char *regName();

    bool isExternal() { return externalInputMask || externalOutputMask || throughputMask; }
};

class RegisterUseList {
public:
    RegisterUseList();
    ~RegisterUseList();
    bool clear();
    RegisterUse *addRegisterUse(RegisterEntry *reg);
    bool append(RegisterUse *ru);
    bool remove(RegisterUse *ru);
    bool hasConsistentCount();
    bool hasEntries();
    void dumpRegisterUses();
    int dumpRegisterClassUses(int regClass);
    int countUncoveredRegisterClassUses(int regClass);
    void dumpVirtualRegisterUses();
    void dumpRegisterClassAllocations(int regClass);
    RegisterUse *findByRegister(RegisterEntry *reg);
    RegisterUse *firstReal();
    RegisterUse *firstVirtual();
    bool optReset();

    RegisterUse *find(RegisterEntry *reg)
    {
        if (!reg)
            return 0;
        RegisterUse *p = head;
        while (p) {
            if (reg == p->registerEntry)
                break;
            p = p->next;
        }
        return p;
    }

    int count;
    RegisterUse *head;
    RegisterUse *tail;
};

struct ScheduleRow {
    wtEntryInl entries[2];
    char skip;
    unsigned char maxStallLength;
    unsigned char stallTypeMask;
    char reservedWorkRowStallByte;
    short cycleTime;

    ScheduleRow();
    ~ScheduleRow();
    bool clearEntries();
    void refreshInstructionOperands(int);
    Instr *getEntry(int p);
    int ctime() { return cycleTime; }
    int countEntries();   // count of used entries
    Instr *onlyEntry();
    bool clearStalls();
    void resetStallMetadata();
    bool addStall(int type, int len);
    int outputMaskFor(RegisterEntry *registerEntry, unsigned char mask);
    int outputMask(RegisterEntry *reg) { return outputMaskFor(reg, 0xff); }
    int inputMaskFor(RegisterEntry *registerEntry, unsigned char mask);
    unsigned int getMinimumExtraLatencyAge();
    char *emit(char *outputBuffer, int flags, bool upper, bool unused);
};

struct InstructionList {
    Instr *head;
    Instr *tail;
    int count;

    InstructionList() { tail = 0; count = 0; head = 0; }
    void reset() { tail = 0; count = 0; head = 0; }
    Instr *first() { return head; }
    Instr *last() { return tail; }
    ~InstructionList() { tail = 0; head = 0; count = 0; }
};

inline const char *codeModeStr(int mode)
{
    switch (mode) {
    case VCL_CODE_INVALID: return "CODE_MODE_BAD";
    case VCL_CODE_UTA: return "CODE_MODE_UTA";
    case VCL_CODE_MID: return "CODE_MODE_MID";
    case VCL_CODE_RAW: return "CODE_MODE_RAW";
    case VCL_CODE_SYNTHETIC: return "CODE_MODE_SYNTH";
    case VCL_CODE_RAW_PREEXPANDED: return "CODE_MODE_RAW_PREEXP";
    case VCL_CODE_PASSTHROUGH: return "CODE_MODE_PASS_THRU";
    }
    return "CODE_MODE_????";
}

class ScheduleTable {
public:
    int optimizerIndex;
    short maximumDynamicStartRow;
    short resetOnlyDynamicStatisticsMarker;  // dynamic statistics high-watermark (written as a short)
    VclAllocationModeByte allocationMode;
    VclCodeModeByte codeModeValue;
    VclRawSchedulingModeByte rawSchedulingMode;
    bool isAbstract() { return codeModeValue == VCL_CODE_UTA; }
    VclCodeModeByte codeMode() { return codeModeValue; }
    char postOptimizationPrepared;
    int declaredRawLength;
    InstructionStream *ownerStream;
    RegisterUseList registerUses;
    int registerLockListCount;
    RegisterPartLockList *registerLockLists;
    int rowCapacity;
    ScheduleRow *rows;
    int rangeStart;
    int rangeEnd;
    int loopSize;
    int mainLoopId;
    int nextInsertionSerial;
    int externalMinimumLength;
    char reservedWorkTableAllocationBytes[4];
    Instr *moveListHead;
    int pendingMoveCount;
    InstructionPosition lastScheduledPosition;
    SchedulingStatistics statistics;
    short streamDependencyLength;
    short streamLogicalLength;
    short logicalInstructionLength;
    short minimumSafeLoopCount;
    short recordedMaximumBranchLoopId;
    char reservedWorkTableLoopBytes[2];
    short fallbackAdjustedLoopLimit;
    short fallbackAbsoluteLoopLimit;
    Instr *loopBoundaryInstruction;
    InstructionList linearInstructions;     // linear list
    InstructionList pendingInstructions;
    InstructionList completedInstructions;
    char fallbackSchedulingActive;
    char movePipeCapacityExceeded;
    char reservedWorkTableQueueBytes[2];
    int vectorMoveCount;             // move count, class 3
    int oddPipeMoveCount;             // move count, odd class
    int evenPipeMoveCount;             // move count, even class
    int dualPipelineInstructionCount;
    int instructionCountsByPipe[3];          // per-pipe instr counts, [2] = total in table

    ScheduleRow *getRowByIndex(int i);
    ScheduleRow *rowAt(int i)
    {
        ScheduleRow *r;
        if (!rows)
            r = 0;
        else if (i < 0 || i >= rowCapacity)
            r = 0;
        else
            r = &rows[i];
        return r;
    }
    ScheduleTable();
    ~ScheduleTable();
    Instr *getBranch();
    bool removeBranch();
    bool unlinkAndUnlog(Instr *in);
    bool logUnlinkFromLinear(Instr *in);
    int getTerminatingConditionalLoopId();
    void dumpSynthetics();
    bool takeOut(Instr *i);
    bool putIn(Instr *i);
#ifdef VCL_POSTOPT_FLOAT_LENGTH
    float getRawOrSyntheticLogicalLength();
#else
    int getRawOrSyntheticLogicalLength();
#endif
    int getLogicalLen();
    int getPhysicalLen();
    void dump();
    bool reserveEntry(int key, int row, int slot);
    Instr *getConditionalBranch();
    const char *ownerName();
    bool hasSafeTrailingInstructions();
    bool dispose();
    bool resetFlowStalls();
    char *emit(char *outputBuffer, int a, int *p1, int *p2);
    int getTableRawMode();
    int getRangeStart();    // range start
    int getLoopSize();    // lsz
    int getRangeEnd();    // range end
    bool grow(int n);
    bool replaceInstruction(Instr *old, Instr *nw);
    bool destroyInstruction(Instr *&in);
    bool prepRawTable();
    bool setTableRawMode(unsigned char mode);
    bool init(unsigned char mode, int size);
    bool changeMode(unsigned char mode);
    bool clearClones();
    Instr *getInstr(int rl, int p);
    Instr *getInstrInRange(int rl, int p);
    int getPipeCount(int p);
    void resetCounts();
    int getCurrentDoneSize();
    bool getCurrentLidStats(int *maxLid, int *maxBranchLid, int *minSfx1, int *minSfx2);
    bool hasEmptyRange();
    bool setRange();
    bool setRange(int len);
    bool setRange(int s, int e);
    bool setSkip(int row, char skip);
    bool logAddLinear_After(Instr *after, Instr *n);
    bool logAddLinear_Head(Instr *n);
    bool logAddLinear_Tail(Instr *n);
    bool makeLinearList();
    bool relinkRawIt();
    bool lockLine(InstructionPosition *line);
    bool unlockLine(InstructionPosition *line);
    bool queueRegisterUse(RegisterUse *ru);
    RegisterPartLockList *getLockList(RegisterEntry *reg, int part);
    bool queueInstruction(Instr *in);
    bool forceInsert(Instr *in, InstructionPosition *position);
    bool doPostOpt(int rawMode);
    ScheduleTable *localClone();
    int ctime(int l)
    {
        if (l < rangeStart || l >= rangeEnd)
            exitWithError("nop line is bad\n");
        return rows[l].cycleTime;
    }
    void clearLinear()
    {
        linearInstructions.reset();
        instructionCountsByPipe[2] = 0;
        dualPipelineInstructionCount = 0;
        vectorMoveCount = 0;
        oddPipeMoveCount = 0;
        instructionCountsByPipe[0] = 0;
        evenPipeMoveCount = 0;
        instructionCountsByPipe[1] = 0;
    }
};

const char *codeModeName(int mode);
const char *rawModeName(int mode);
const char *allocModeName(unsigned int mode);

bool cloneAppend(ScheduleTable *from, ScheduleTable *to, int lidS, int lidE,
                 bool includeBranches, bool adjustCloneAliasLoopId, bool includeNops);
extern Instr *earliestInsertionInstruction;   // instr with smallest insertionSerial
void trackEarliestInsertion(Instr *in);

#endif

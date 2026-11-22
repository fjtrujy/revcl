// Work table: per-register use lists (names guessed), CIE group 6 owns the lookup.
#ifndef WORKTABLE_H
#define WORKTABLE_H

#include "os.h"
#include "Instr.h"
#include "tPairList.h"
#include "regUseSlot.h"
#include "wtEntry.h"

struct ol_s;
class instStream;
struct RegPoolEntry;

struct regUse {
    regUse *previous;
    regUse *next;
    RegPoolEntry *registerEntry;
    unsigned char externalInputMask;
    unsigned char externalOutputMask;
    unsigned char inputMask;
    unsigned char outputMask;
    unsigned char throughputMask;
    bool unk_11;
    unsigned char hazardMask;
    unsigned char unk_13;
    unsigned char hazardBroadcastMask;
    char externalBlockMode;
    short unk_16;
    short unk_18;
    Instr *definingInstruction;
    regUseSlot slots[4];

    regUse(RegPoolEntry *reg);
    ~regUse();
    void validateAndDumpParts();
    regUse *nextReal();
    regUse *nextVirtual();
    bool optReset();
    regUse *Clone();
    const char *regName();

    bool isExternal() { return externalInputMask || externalOutputMask || throughputMask; }
};

class regUseList {
public:
    regUseList();
    ~regUseList();
    bool clear();
    regUse *AddNewRegRef(RegPoolEntry *reg);
    bool append(regUse *ru);
    bool remove(regUse *ru);
    bool hasConsistentCount();
    bool hasEntries();
    void dumpRegisterUses();
    int dumpRegisterClassUses(int regClass);
    int countUncoveredRegisterClassUses(int regClass);
    void dumpVirtualRegisterUses();
    void dumpRegisterClassAllocations(int regClass);
    regUse *findByRegister(RegPoolEntry *reg);
    regUse *firstReal();
    regUse *firstVirtual();
    bool optReset();

    regUse *find(RegPoolEntry *reg)
    {
        if (!reg)
            return 0;
        regUse *p = head;
        while (p) {
            if (reg == p->registerEntry)
                break;
            p = p->next;
        }
        return p;
    }

    int count;
    regUse *head;
    regUse *tail;
};

struct wtRow {
    wtEntryInl entries[2];
    char skip;
    unsigned char maxStallLength;
    unsigned char stallTypeMask;
    char unk_0b;
    short cycleTime;

    wtRow();
    ~wtRow();
    bool clearEntries();
    void refreshInstructionOperands(int);
    Instr *getEntry(int p);
    int ctime() { return cycleTime; }
    int countEntries();   // count of used entries
    Instr *onlyEntry();
    bool clearStalls();
    void resetStallMetadata();
    bool addStall(int type, int len);
    int outputMaskFor(RegPoolEntry *reg, unsigned char mask);
    int outputMask(RegPoolEntry *reg) { return outputMaskFor(reg, 0xff); }
    int inputMaskFor(RegPoolEntry *reg, unsigned char mask);
    unsigned int getMinimumExtraLatencyAge();
    char *emit(char *buf, int flags, bool upper, bool unused);
};

struct wtInstrList {
    Instr *head;
    Instr *tail;
    int count;

    wtInstrList() { tail = 0; count = 0; head = 0; }
    void reset() { tail = 0; count = 0; head = 0; }
    Instr *first() { return head; }
    Instr *last() { return tail; }
    ~wtInstrList() { tail = 0; head = 0; count = 0; }
};

inline const char *codeModeStr(int mode)
{
    switch (mode) {
    case 0: return "CODE_MODE_BAD";
    case 1: return "CODE_MODE_UTA";
    case 2: return "CODE_MODE_MID";
    case 3: return "CODE_MODE_RAW";
    case 4: return "CODE_MODE_SYNTH";
    case 5: return "CODE_MODE_RAW_PREEXP";
    case 6: return "CODE_MODE_PASS_THRU";
    }
    return "CODE_MODE_????";
}

class workTable {
public:
    int optimizerIndex;
    short unk_04;
    short unk_06;  // dynamic statistics high-watermark (written as a short)
    char allocationMode;
    unsigned char codeModeValue;
    unsigned char rawSchedulingMode;
    bool isAbstract() { return codeModeValue == 1; }
    unsigned char codeMode() { return codeModeValue; }
    char unk_0b;
    int declaredRawLength;
    instStream *ownerStream;
    regUseList registerUses;
    int registerLockListCount;
    cls0805c244 *registerLockLists;
    int rowCapacity;
    wtRow *rows;
    int rangeStart;
    int rangeEnd;
    int loopSize;
    int mainLoopId;
    int unk_40;
    int externalMinimumLength;
    char unk_48[4];
    Instr *moveListHead;
    int pendingMoveCount;
    cls08058718 lastScheduledPosition;
    cls0805e2e4 statistics;
    short unk_74;
    short unk_76;
    short logicalInstructionLength;
    short unk_7a;
    short unk_7c;
    char unk_7e[2];
    short unk_80;
    short unk_82;
    Instr *unk_84;
    wtInstrList linearInstructions;     // linear list
    wtInstrList pendingInstructions;
    wtInstrList completedInstructions;
    char unk_ac;
    char unk_ad;
    char unk_ae[2];
    int vectorMoveCount;             // move count, class 3
    int oddPipeMoveCount;             // move count, odd class
    int evenPipeMoveCount;             // move count, even class
    int dualPipelineInstructionCount;
    int instructionCountsByPipe[3];          // per-pipe instr counts, [2] = total in table

    wtRow *getRowByIndex(int i);
    wtRow *rowAt(int i)
    {
        wtRow *r;
        if (!rows)
            r = 0;
        else if (i < 0 || i >= rowCapacity)
            r = 0;
        else
            r = &rows[i];
        return r;
    }
    workTable();
    ~workTable();
    Instr *getBranch();
    bool removeBranch();
    bool unlinkAndUnlog(Instr *in);
    bool logUnlinkFromLinear(Instr *in);
    int getCondLid();
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
    char *emit(char *buf, int a, int *p1, int *p2);
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
    bool lockLine(cls08058718 *line);
    bool unlockLine(cls08058718 *line);
    bool qRegRef(regUse *ru);
    cls0805c244 *getLockList(RegPoolEntry *reg, int part);
    bool qInstruction(Instr *in);
    bool forceInsert(Instr *in, cls08058718 *pos);
    bool doPostOpt(int rawMode);
    workTable *localClone();
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

bool cloneAppend(workTable *from, workTable *to, int lidS, int lidE, bool a5, bool a6, bool a7);
extern Instr *earliestInsertionInstruction;   // instr with smallest insertionSerial
void trackEarliestInsertion(Instr *in);

#endif

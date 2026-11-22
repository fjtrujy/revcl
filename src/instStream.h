// Instruction stream, owning scheduling state, links, labels and instructions.
#ifndef INSTSTREAM_H
#define INSTSTREAM_H

#include "os.h"
#include "mList.h"
#include "workTable.h"
#include "Instr.h"
#include "statCounts.h"
#include "linkList.h"

class ScheduleTable;
class Instr;
struct RegisterUse;
class InstructionStream;
struct Program;

// 16-byte record of an instruction stream's optimization candidate.
struct StreamOptimizationRecord {
    int instructionCount;
    int optimalLength;
    int mainLoopId;
    int candidateLoopId;
};

class InstructionStream {
public:
    InstructionStream();
    const char *streamName();  // stream name or default
    ScheduleTable *getWorkTable()
    {
        if (workTablePtr)
            return workTablePtr;
        reportError("work table not set in %s\n", streamName());
        exitAfterCleanup(-1);
        return workTablePtr;
    }
    // Same check as getWorkTable, written as a guarded block (different code shape).
    ScheduleTable *getTable()
    {
        if (!workTablePtr) {
            reportError("work table not set in %s\n", streamName());
            exitAfterCleanup(-1);
        }
        return workTablePtr;
    }

    ScheduleTable *checkedTable(const char *msg)
    {
        if (workTablePtr)
            return workTablePtr;
        reportError(msg);
        exitAfterCleanup(-1);
        return workTablePtr;
    }
    void requireTable(const char *msg)
    {
        if (!workTablePtr) {
            reportError(msg);
            exitAfterCleanup(-1);
        }
    }

    bool gatherStats();
    bool buildForwardDependencies(bool a, bool b);  // build memory dependencies
    bool flowReset();
    const char *branchTypeName();
    const char *endTypeName();
    unsigned char getEndType() { return endType; }
    const char *startTypeName();
    char *emit(char *outputBuffer, int a, int *p1, int *p2);
    int checkStartEnd()
    {
        int st = startType;
        int ok = 0;
        if (st == 1 || st == 2) {
            int et = endType;
            unsigned endTypeCount = 4;
            ok = (unsigned)(et - 1) < endTypeCount;
        }
        return ok;
    }
    unsigned char getCodeMode(); // code mode
    unsigned char getRawSchedulingMode(); // work table mode
    int countUnresolvedExternalInputs();
    int countUnresolvedExternalOutputs();
    const char *getCodeModeName();
    void dumpSynthetics();
    char *emitLabels(char *outputBuffer);
    Instr *getBranch();
    bool removeBranch();
    char *getBranchTargetImmediate();
    RegisterEntry *getBranchTargetRegister();
    ~InstructionStream();
    bool reset();
    void dump(int);
    void zeroStats();
    bool iOwn(Instr *i);
    bool patchLatencies();
    bool canLoop();
    bool usesFreeRawLayout();         // raw-mode work table
    bool usesFixedRawLayout();
    unsigned char getAllocMode();
    bool isOutsideRawMode();         // not raw mode
    bool hasSafeTrailingInstructions();
    int checkStartEndStatus();          // out-of-line checkStartEnd
    bool disposeInputTable();
    bool processCmdSynth(Instr *i);
    void copySourceRecordsFrom(InstructionStream *sourceStream);  // copy record table from src
    bool setOptTable(ScheduleTable *nt);

    int optimizationRecordCount;
    StreamOptimizationRecord optimizationRecords[16];
    Program *owningProgram;
    InstructionStream *nextStream;         // next in Program::completedInstructions
    InstructionStream *previousStream;         // previous stream (to-register branch)
    mList labels;               // name list
    char *passThroughText;               // pass thru
    StreamLinkList inputLinks;            // input links
    StreamLinkList outputLinks;            // output links
    char hasLoopOptimization;
    char reservedStreamFlagsAfterLoopOptimization[3];
    int appliedOptimizationMask;
    ScheduleTable sourceTable;           // own work table
    ScheduleTable *optimizedTable;  // opt table
    ScheduleTable *workTablePtr;  // work table
    ScheduleTable *emissionTable;  // tidied work table
    ScheduleTable *savedTable;  // saved work table
    short outputLocationCount;
    unsigned char visitState;      // visit mode (0 unvisited, 1 active, 2 visited, 3 end match, 4 dead end)
    char pendingHazardBroadcast;
    char hazardFlag;               // hazard flag
    char hazardVisited;               // visited flag in hazard broadcast
    char reservedStreamHazardBytes[2];
    int backtraceDependencyLength;                // dependency length used by back trace
    short earliestOutputLocation;
    short latestOutputLocation;
    RegisterUse *backtraceRegisterUse;
    unsigned *traceStore;          // m_tStore
    int destinationKey;                // dst key
    int minimumPathLength;
    InstructionStream *shortestPathPredecessor;        // min-path predecessor
    int maximumPathLength;
    InstructionStream *longestPathPredecessor;        // max-path predecessor
    InstructionStream *nextSourceStream;
    InstructionStream *nextMatchedStream;
    InstructionStream *nextResolvedStream;
    InstructionStream *nextActiveStream;
    char participatesInStall;      // set when stream takes part in a stall
    char allowUpwardReduction;
    char allowDownwardReduction;
    char reductionSkipped;
    char reachableFromEntry;      // reached top (accessible)
    char mergeIntoSuccessor;      // collapse into successor (tidy)
    char reservedStreamReductionBytes[2];
    int absoluteLoopCount;
    int extraLoopCount;
    int knownMinimumLoopCount;                 // known min count
    int safeMinimumLoopCount;                 // minimum safe count
    SchedulingStatistics schedulingStatistics;
    statCounts stats;
    short branchForwardLatencyDepth;
    short resetOnlyStreamBranchTimingSentinel;
    short savedStallReason;
    short currentStallReason;
    int physicalInstructionBudget;
    short streamId;     // stream id
    short streamSerial;     // stream serial
    short loopRegionRole;
    short resetOnlyStreamLoopShort;
    int emissionStartIndex;
    unsigned char startType;  // start type (ISTST_*)
    unsigned char endType;  // end type (ISTET_*; 2 adds 1000 to back trace dep length)
    unsigned char branchType;  // branch type
    char reservedStreamDirectiveByte;
    char *endDirectiveText;
    char leadingLabelFlags;
    char trailingLabelFlags;
    char containsBarrier;
    char mergedBarrierFlag;
    int resetOnlyStreamEmissionWord;
    char externalSchedulingPrepared;
    char reservedStreamEmissionBytes[3];
    int dependencyLength;       // dependency length
    int logicalLength;

    int getRecordedDependencyLength();
    int getRecordedLogicalLength();
    int getOrGuessDepLength();
    int getClampedDependencyLength();
    int getOrGuessLogicalLength();
};

#endif

// Instruction stream (owner of instructions); layout mostly unknown.
#ifndef INSTSTREAM_H
#define INSTSTREAM_H

#include "os.h"
#include "mList.h"
#include "workTable.h"
#include "Instr.h"
#include "statCounts.h"
#include "linkList.h"

class workTable;
class Instr;
struct regUse;
class instStream;
struct Program;

// 16-byte record in the leading table of a stream (name guessed).
struct istRec {
    int unk_00;
    int unk_04;
    int unk_08;
    int unk_0c;
};

class instStream {
public:
    instStream();
    const char *streamName();  // stream name or default
    workTable *getWorkTable()
    {
        if (workTablePtr)
            return workTablePtr;
        reportError("work table not set in %s\n", streamName());
        exitAfterCleanup(-1);
        return workTablePtr;
    }
    // Same check as getWorkTable, written as a guarded block (different code shape).
    workTable *getTable()
    {
        if (!workTablePtr) {
            reportError("work table not set in %s\n", streamName());
            exitAfterCleanup(-1);
        }
        return workTablePtr;
    }

    workTable *checkedTable(const char *msg)
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
    char *emit(char *buf, int a, int *p1, int *p2);
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
    int getUninsertedxInputs();
    int getUninsertedxOutputs();
    const char *getCodeModeName();
    void dumpSynthetics();
    char *emitLabels(char *buf);
    Instr *getBranch();
    bool removeBranch();
    char *getBranchTargetImmediate();
    RegPoolEntry *getBranchTargetRegister();
    ~instStream();
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
    bool func_0806da58();
    int checkStartEndStatus();          // out-of-line checkStartEnd
    bool disposeInputTable();
    bool processCmdSynth(Instr *i);
    void copySourceRecordsFrom(instStream *src);  // copy record table from src
    bool setOptTable(workTable *nt);

    int unk_00;                  // record count
    istRec unk_04[16];
    Program *owningProgram;
    instStream *nextStream;         // next in Program::completedInstructions
    instStream *previousStream;         // previous stream (to-register branch)
    mList labels;               // name list
    char *passThroughText;               // pass thru
    linkList inputLinks;            // input links
    linkList outputLinks;            // output links
    char unk_140;
    char unk_141[3];
    int unk_144;
    workTable sourceTable;           // own work table
    workTable *optimizedTable;  // opt table
    workTable *workTablePtr;  // work table
    workTable *emissionTable;  // tidied work table
    workTable *savedTable;  // saved work table
    short unk_224;
    unsigned char visitState;      // visit mode (0 unvisited, 1 active, 2 visited, 3 end match, 4 dead end)
    char unk_227;
    char hazardFlag;               // hazard flag
    char hazardVisited;               // visited flag in hazard broadcast
    char unk_22a[2];
    int backtraceDependencyLength;                // dependency length used by back trace
    short unk_230;
    short unk_232;
    regUse *unk_234;
    unsigned *traceStore;          // m_tStore
    int destinationKey;                // dst key
    int unk_240;
    instStream *shortestPathPredecessor;        // min-path predecessor
    int unk_248;
    instStream *longestPathPredecessor;        // max-path predecessor
    instStream *unk_250;        // next in Program::unk_dc
    instStream *unk_254;        // next in Program::unk_d8
    instStream *unk_258;        // next in Program::unk_e0
    instStream *unk_25c;        // next in Program::unk_e4
    char participatesInStall;      // set when stream takes part in a stall
    char unk_261;
    char unk_262;
    char unk_263;
    char reachableFromEntry;      // reached top (accessible)
    char mergeIntoSuccessor;      // collapse into successor (tidy)
    char unk_266[2];
    int unk_268;
    int unk_26c;
    int knownMinimumLoopCount;                 // known min count
    int safeMinimumLoopCount;                 // minimum safe count
    cls0805e2e4 unk_278;
    statCounts stats;
    short unk_2b4;
    short unk_2b6;
    short unk_2b8;
    short unk_2ba;
    int unk_2bc;
    short streamId;     // stream id
    short streamSerial;     // stream serial
    short unk_2c4;
    short unk_2c6;
    int unk_2c8;
    unsigned char startType;  // start type (ISTST_*)
    unsigned char endType;  // end type (ISTET_*; 2 adds 1000 to back trace dep length)
    unsigned char branchType;  // branch type
    char unk_2cf;
    char *endDirectiveText;
    char unk_2d4;      // dbgTop
    char unk_2d5;      // dbgEnd
    char unk_2d6;
    char unk_2d7;
    int unk_2d8;
    char unk_2dc;
    char unk_2dd[3];
    int dependencyLength;       // dependency length
    int logicalLength;

    int getRecordedDependencyLength();
    int getRecordedLogicalLength();
    int getOrGuessDepLength();
    int getClampedDependencyLength();
    int getOrGuessLogicalLength();
};

#endif

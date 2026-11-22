#ifndef WTQUEUE_SHARED_H
#define WTQUEUE_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "Block.h"
#include "instStream.h"
#include "Program.h"

typedef bool (*wrangleFunc)(ScheduleTable *, int *);

struct optimiser {
    const char *name;
    int wmode;
    int flags;
    wrangleFunc func;
    int reservedOptimizerWord;
};

extern optimiser optTable[11];
extern const char *currentOptimizerName;

InstructionStream *selectUninsertedStream(Program *p);
bool prepExtInfo(Program *p, InstructionStream *s, bool b);
ScheduleTable *prepUta(Program *p, InstructionStream *s, int loopSize, char smart);
bool statReset(Program *p);
bool dynCalcStats(ScheduleTable *wt);
bool scheduleMidTableMode1(ScheduleTable *wt, wrangleFunc f);
bool scheduleMidTableMode0(ScheduleTable *wt, wrangleFunc f);
bool reduceExtents(ScheduleTable *wt);
bool adjustTrailingBranchLayout(ScheduleTable *wt);
bool assignLocalRegisters(Program *p, ScheduleTable *wt);
bool makeLpWork(InstructionStream *s, int maxSpread);
bool disposeLpWork(InstructionStream *s);
bool keepLpWork(InstructionStream *s);
bool optReset(Program *p);
ScheduleTable *makeForceTable(Program *p, InstructionStream *s);
void dumpProgramStreamsAndRegisters(Program *p, int a);
const char *stallReasonStr(short why);
bool sredTable(ScheduleTable *wt);
bool prepareReductionCandidates(Program *p, InstructionStream *s, ScheduleTable *wt);
bool reduceLinearSchedule(Program *p, ScheduleTable *wt, unsigned char a, unsigned char b);
bool unlinkFromCompletedInstructions(Block *block, Instr *n);
bool setRange(int s, int e, int lsz);

#endif

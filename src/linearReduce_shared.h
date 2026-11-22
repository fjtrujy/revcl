#ifndef LINEARREDUCE_SHARED_H
#define LINEARREDUCE_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"

class Program;

int setRangeFrom(ScheduleTable *t);
bool moveLocks(Instr *i, InstructionPosition *pos);
int checkPlacementInAllowedPipelines(ScheduleTable *wt, Instr *in, InstructionPosition *position, bool upwards);
void dumpProgramStreamsAndRegisters(Program *p, int a);
int earliestInsert(ScheduleTable *wt, Instr *in, bool *found);
int checkInstructionPlacementConstraints(ScheduleTable *wt, Instr *i, InstructionPosition *position);
bool setRange(int start, int end, int lsz);
int pullUp(ScheduleTable *wt);
int dragDown(ScheduleTable *wt);
bool doLocalMove(ScheduleTable *wt, Instr *in, int s_test, int e_test, int dl);

static inline bool isPipeMode(ScheduleTable *wt)
{
    return wt->codeModeValue == VCL_CODE_RAW && (wt->rawSchedulingMode == VCL_RAW_COMPILER_FREE || wt->rawSchedulingMode == VCL_RAW_USER_FREE);
}

#endif

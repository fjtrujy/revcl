#ifndef LINEARREDUCE_SHARED_H
#define LINEARREDUCE_SHARED_H

#include "os.h"
#include "workTable.h"
#include "Instr.h"

class Program;

int setRangeFrom(workTable *t);
bool moveLocks(Instr *i, cls08058718 *pos);
int checkPlacementInAllowedPipelines(workTable *wt, Instr *in, cls08058718 *pos, bool upwards);
void dumpProgramStreamsAndRegisters(Program *p, int a);
int earliestInsert(workTable *wt, Instr *in, bool *found);
int checkInstructionPlacementConstraints(workTable *wt, Instr *i, cls08058718 *pos);
bool setRange(int start, int end, int lsz);
int pullUp(workTable *wt);
int dragDown(workTable *wt);
bool doLocalMove(workTable *wt, Instr *in, int s_test, int e_test, int dl);

static inline bool isPipeMode(workTable *wt)
{
    return wt->codeModeValue == 3 && (wt->rawSchedulingMode == 1 || wt->rawSchedulingMode == 3);
}

#endif

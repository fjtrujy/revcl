#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int optimizationTimeoutSeconds;


bool selectDumbSchedulingCandidate(workTable *wt, wrangleCand *c)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *in = wt->moveListHead; in; in = in->nextMoveInstruction) {
        if (in->linearOrderIndex < min) {
            best = in;
            min = in->linearOrderIndex;
        }
    }
    if (!best)
        return false;
    c->candidateInstruction = best;
    c->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    if (!best->moveRecordWord && c->candidateRow < wt->lastScheduledPosition.getRow())
        c->candidateRow = wt->lastScheduledPosition.getRow();
    c->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

bool selectLoopDumbSchedulingCandidate(workTable *wt, wrangleCand *c)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *in = wt->moveListHead; in; in = in->nextMoveInstruction) {
        if (in->linearOrderIndex < min) {
            best = in;
            min = in->linearOrderIndex;
        }
    }
    if (!best)
        return false;
    c->candidateInstruction = best;
    c->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    if (!best->moveRecordWord && c->candidateRow < wt->lastScheduledPosition.getRow() - 64)
        c->candidateRow = wt->lastScheduledPosition.getRow() - 64;
    if (c->candidateRow < 0)
        c->candidateRow = 0;
    c->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

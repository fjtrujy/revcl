#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(workTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int timeout_080b3010;


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
    c->unk_00 = best;
    c->unk_04 = best->candidateBaseRow + best->candidateCostOffset;
    if (!best->moveRecordWord && c->unk_04 < wt->unk_54.getRow())
        c->unk_04 = wt->unk_54.getRow();
    c->unk_06 = best->candidateAttemptCount & 1;
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
    c->unk_00 = best;
    c->unk_04 = best->candidateBaseRow + best->candidateCostOffset;
    if (!best->moveRecordWord && c->unk_04 < wt->unk_54.getRow() - 64)
        c->unk_04 = wt->unk_54.getRow() - 64;
    if (c->unk_04 < 0)
        c->unk_04 = 0;
    c->unk_06 = best->candidateAttemptCount & 1;
    return true;
}

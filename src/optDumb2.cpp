#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

bool optDumb2(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->moveListHead;
    Instr *best = 0;
    int bestV = 0x7fffffff;
    for (; in; in = in->nextMoveInstruction) {
        if (in->linearOrderIndex < bestV) {
            best = in;
            bestV = in->linearOrderIndex;
        }
    }
    if (!best)
        return false;
    c->candidateInstruction = best;
    c->candidateRow = (best->candidateAttemptCount >> 1) + best->candidateBaseRow;
    if (!best->moveRecordWord) {
        int row = wt->lastScheduledPosition.getRow();
        if (c->candidateRow < (short)row)
            c->candidateRow = row;
    }
    c->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

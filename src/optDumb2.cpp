#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

bool optDumb2(workTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    Instr *best = 0;
    int bestV = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        if (instruction->linearOrderIndex < bestV) {
            best = instruction;
            bestV = instruction->linearOrderIndex;
        }
    }
    if (!best)
        return false;
    candidate->candidateInstruction = best;
    candidate->candidateRow = (best->candidateAttemptCount >> 1) + best->candidateBaseRow;
    if (!best->moveRecordWord) {
        int row = scheduleTable->lastScheduledPosition.getRow();
        if (candidate->candidateRow < (short)row)
            candidate->candidateRow = row;
    }
    candidate->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

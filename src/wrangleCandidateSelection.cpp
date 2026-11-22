#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "Block.h"
#include "wrangle.h"

typedef bool (*wrangleFunc)(ScheduleTable *, int *);

void adjustMoveCount(Block *blk, Instr *mv, int n);
extern int optimizationTimeoutSeconds;


bool selectDumbSchedulingCandidate(ScheduleTable *scheduleTable, wrangleCand *candidate)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *instruction = scheduleTable->moveListHead; instruction; instruction = instruction->nextMoveInstruction) {
        if (instruction->linearOrderIndex < min) {
            best = instruction;
            min = instruction->linearOrderIndex;
        }
    }
    if (!best)
        return false;
    candidate->candidateInstruction = best;
    candidate->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    if (!best->moveRecordWord && candidate->candidateRow < scheduleTable->lastScheduledPosition.getRow())
        candidate->candidateRow = scheduleTable->lastScheduledPosition.getRow();
    candidate->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

bool selectLoopDumbSchedulingCandidate(ScheduleTable *scheduleTable, wrangleCand *candidate)
{
    Instr *best = 0;
    int min = 0x7fffffff;
    for (Instr *instruction = scheduleTable->moveListHead; instruction; instruction = instruction->nextMoveInstruction) {
        if (instruction->linearOrderIndex < min) {
            best = instruction;
            min = instruction->linearOrderIndex;
        }
    }
    if (!best)
        return false;
    candidate->candidateInstruction = best;
    candidate->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    if (!best->moveRecordWord && candidate->candidateRow < scheduleTable->lastScheduledPosition.getRow() - 64)
        candidate->candidateRow = scheduleTable->lastScheduledPosition.getRow() - 64;
    if (candidate->candidateRow < 0)
        candidate->candidateRow = 0;
    candidate->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

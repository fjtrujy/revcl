#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int normalRegCost(Instr *in);
int hDownRegCost(Instr *in);
int vutaRegCost(Instr *in);
int anotherRegCost(Instr *in);
int normal2RegCost(Instr *in);
int vuta1RegCost(Instr *in);


bool optNormal1(workTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        int a = instruction->schedulingStatistics.reverseLatencyDepth - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardLatencyDepth;
        if (a > bestA) {
            best = instruction;
            bestA = a;
            bestB = instruction->schedulingStatistics.moveDistance - instruction->candidateAttemptCount;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int e = instruction->schedulingStatistics.moveDistance - instruction->candidateAttemptCount;
            if (e > bestB) {
                bestB = e;
                bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                best = instruction;
            } else if (e == bestB) {
                int f = instruction->candidateBaseRow + instruction->candidateCostOffset;
                if (f < bestD) {
                    bestD = f;
                    bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                    best = instruction;
                } else if (f == bestD && instruction->schedulingStatistics.reverseDependencyHeight > bestC) {
                    bestC = instruction->schedulingStatistics.reverseDependencyHeight;
                    best = instruction;
                }
            }
        }
    }
    if (!best)
        return false;
    candidate->candidateInstruction = best;
    candidate->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    candidate->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

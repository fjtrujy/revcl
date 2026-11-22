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
        int latencyScore = instruction->schedulingStatistics.reverseLatencyDepth - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardLatencyDepth;
        if (latencyScore > bestA) {
            best = instruction;
            bestA = latencyScore;
            bestB = instruction->schedulingStatistics.moveDistance - instruction->candidateAttemptCount;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (latencyScore == bestA) {
            int distanceScore = instruction->schedulingStatistics.moveDistance - instruction->candidateAttemptCount;
            if (distanceScore > bestB) {
                bestB = distanceScore;
                bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                best = instruction;
            } else if (distanceScore == bestB) {
                int candidatePlacementCost = instruction->candidateBaseRow + instruction->candidateCostOffset;
                if (candidatePlacementCost < bestD) {
                    bestD = candidatePlacementCost;
                    bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                    best = instruction;
                } else if (candidatePlacementCost == bestD && instruction->schedulingStatistics.reverseDependencyHeight > bestC) {
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

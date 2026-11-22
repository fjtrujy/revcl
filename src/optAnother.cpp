#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int estimateNormalRegisterCost(Instr *in);
int estimateHeightDownRegisterCost(Instr *in);
int vutaRegCost(Instr *in);
int anotherRegCost(Instr *in);
int normal2RegCost(Instr *in);
int vuta1RegCost(Instr *in);


bool optAnother(ScheduleTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    int bestA = 0x80000000;
    Instr *best = 0;
    int bestB = 0x80000000, bestC = 0x80000000, bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        int cost = anotherRegCost(instruction);
        int distanceScore = instruction->schedulingStatistics.moveDistance - instruction->candidateAttemptCount + instruction->schedulingStatistics.reverseLatencyDepth + cost;
        if (distanceScore > bestA) {
            best = instruction;
            bestA = distanceScore;
            int moveCountScore = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount;
            int dependencyHeight = instruction->schedulingStatistics.forwardDependencyHeight;
            bestB = moveCountScore + (dependencyHeight >> 2) + cost;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (distanceScore == bestA) {
            int secondMoveCountScore = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount;
            int secondDependencyHeight = instruction->schedulingStatistics.forwardDependencyHeight;
            int weightedScore = secondMoveCountScore + (secondDependencyHeight >> 2) + cost;
            if (weightedScore > bestB) {
                bestB = weightedScore;
                bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                best = instruction;
            } else if (weightedScore == bestB) {
                int placementCost = instruction->candidateBaseRow + instruction->candidateCostOffset;
                if (placementCost < bestD) {
                    bestD = placementCost;
                    bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                    best = instruction;
                } else if (placementCost == bestD && instruction->schedulingStatistics.reverseDependencyHeight > bestC) {
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

int anotherRegCost(Instr *instruction)
{
    int cost = 0;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        RegisterEntry *registerEntry = instruction->operands[operandIndex].inRegOp2();
        if (registerEntry) {
            switch (registerEntry->registerIndex) {
            case 55:
            case 56:
            case 59:
                cost += 400;
            }
        }
    }
    return cost;
}

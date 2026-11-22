#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int vuta1RegCost(Instr *in);

bool optVuta1(ScheduleTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    int bestA = 0x80000000;
    Instr *best = 0;
    int bestB = 0x80000000, bestC = 0x80000000, bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        int cost = vuta1RegCost(instruction);
        int base = instruction->schedulingStatistics.moveDistance;
        int forwardLatencyScore = base - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardLatencyDepth * 32 + cost;
        if (forwardLatencyScore > bestA) {
            best = instruction;
            bestA = forwardLatencyScore;
            bestB = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardDependencyHeight + cost;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (forwardLatencyScore == bestA) {
            int dependencyScore = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardDependencyHeight + cost;
            if (dependencyScore > bestB) {
                bestB = dependencyScore;
                bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                best = instruction;
            } else if (dependencyScore == bestB) {
                int candidatePlacementCost = instruction->candidateBaseRow + (instruction->candidateAttemptCount >> 2);
                if (candidatePlacementCost < bestD) {
                    bestD = candidatePlacementCost;
                    bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                    best = instruction;
                } else if (candidatePlacementCost == bestD && instruction->schedulingStatistics.reverseLatencyDepth > bestC) {
                    bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                    best = instruction;
                }
            }
        }
    }
    if (!best)
        return false;
    candidate->candidateInstruction = best;
    candidate->candidateRow = (best->candidateAttemptCount >> 2) + best->candidateBaseRow;
    candidate->candidatePipe = (best->candidateAttemptCount >> 3) & 1;
    return true;
}

int vuta1RegCost(Instr *instruction)
{
    int cost = 0;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        RegisterEntry *registerEntry = instruction->operands[operandIndex].inRegOp2();
        if (registerEntry && registerEntry->registerIndex == 59)
            cost += 400;
    }
    return cost;
}

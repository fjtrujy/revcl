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


bool optVuta(workTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        int cost = vutaRegCost(instruction);
        int base = instruction->schedulingStatistics.moveDistance;
        int forwardLatencyScore = base - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardLatencyDepth * 32 + cost;
        if (forwardLatencyScore > bestA) {
            bestA = forwardLatencyScore;
            bestB = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardDependencyHeight + cost;
            best = instruction;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (forwardLatencyScore == bestA) {
            int dependencyScore = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardDependencyHeight + cost;
            if (dependencyScore > bestB) {
                bestB = dependencyScore;
                bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                best = instruction;
            } else if (dependencyScore == bestB) {
                int candidatePlacementCost = instruction->candidateBaseRow + instruction->candidateCostOffset;
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
    candidate->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    candidate->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

int vutaRegCost(Instr *instruction)
{
    int cost = 0;
    for (int operandIndex = 0; operandIndex != instruction->operandCount; operandIndex++) {
        RegPoolEntry *registerEntry = instruction->operands[operandIndex].inRegOp2();
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


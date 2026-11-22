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

bool optNormal(workTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        estimateNormalRegisterCost(instruction);
        int reverseLatencyScore = instruction->schedulingStatistics.reverseLatencyDepth - instruction->candidateAttemptCount;
        if (reverseLatencyScore > bestA) {
            best = instruction;
            bestA = reverseLatencyScore;
            bestB = -instruction->linearOrderIndex;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (reverseLatencyScore == bestA) {
            int linearOrderScore = -instruction->linearOrderIndex;
            if (linearOrderScore > bestB) {
                bestB = linearOrderScore;
                bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                best = instruction;
            } else if (linearOrderScore == bestB) {
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

int estimateNormalRegisterCost(Instr *instruction)
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


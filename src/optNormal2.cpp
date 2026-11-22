#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int normal2RegCost(Instr *in);
int vuta1RegCost(Instr *in);

bool optNormal2(workTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        normal2RegCost(instruction);
        int d = instruction->schedulingStatistics.reverseLatencyDepth - instruction->candidateAttemptCount;
        if (d > bestA) {
            best = instruction;
            bestA = d;
            bestB = -instruction->linearOrderIndex;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (d == bestA) {
            int e = -instruction->linearOrderIndex;
            if (e > bestB) {
                bestB = e;
                bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                best = instruction;
            } else if (e == bestB) {
                int f = instruction->candidateBaseRow + (instruction->candidateAttemptCount >> 2);
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
    candidate->candidateRow = (best->candidateAttemptCount >> 2) + best->candidateBaseRow;
    candidate->candidatePipe = (best->candidateAttemptCount >> 3) & 1;
    return true;
}

int normal2RegCost(Instr *instruction)
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

#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int vuta1RegCost(Instr *in);

bool optVuta1(workTable *scheduleTable, wrangleCand *candidate)
{
    Instr *instruction = scheduleTable->moveListHead;
    int bestA = 0x80000000;
    Instr *best = 0;
    int bestB = 0x80000000, bestC = 0x80000000, bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        int cost = vuta1RegCost(instruction);
        int base = instruction->schedulingStatistics.moveDistance;
        int a = base - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardLatencyDepth * 32 + cost;
        if (a > bestA) {
            best = instruction;
            bestA = a;
            bestB = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardDependencyHeight + cost;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int e = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardDependencyHeight + cost;
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
                } else if (f == bestD && instruction->schedulingStatistics.reverseLatencyDepth > bestC) {
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
        RegPoolEntry *registerEntry = instruction->operands[operandIndex].inRegOp2();
        if (registerEntry && registerEntry->registerIndex == 59)
            cost += 400;
    }
    return cost;
}

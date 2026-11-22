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


bool optHDown(workTable *wt, wrangleCand *c)
{
    Instr *instruction = wt->moveListHead;
    int bestA = 0x80000000;
    Instr *best = 0;
    int bestB = 0x80000000, bestC = 0x80000000, bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        int cost = hDownRegCost(instruction);
        int a = instruction->schedulingStatistics.moveDistance - instruction->candidateAttemptCount + instruction->schedulingStatistics.reverseLatencyDepth + cost;
        if (a > bestA) {
            best = instruction;
            bestA = a;
            int b = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount;
            int h = instruction->schedulingStatistics.forwardDependencyHeight;
            bestB = b + (h >> 2) + cost;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int b = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount;
            int h = instruction->schedulingStatistics.forwardDependencyHeight;
            int e = b + (h >> 2) + cost;
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
                    bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                    best = instruction;
                }
            }
        }
    }
    if (!best)
        return false;
    c->candidateInstruction = best;
    c->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    c->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

int hDownRegCost(Instr *instruction)
{
    int cost = 0;
    for (int i = 0; i != instruction->operandCount; i++) {
        RegPoolEntry *registerEntry = instruction->operands[i].inRegOp2();
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

bool optDUp(workTable *wt, wrangleCand *c)
{
    Instr *instruction = wt->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; instruction; instruction = instruction->nextMoveInstruction) {
        int a = instruction->schedulingStatistics.moveDistance;
        a -= instruction->candidateAttemptCount;
        if (a > bestA) {
            best = instruction;
            bestA = a;
            bestB = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardLatencyDepth;
            bestC = instruction->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int e = instruction->schedulingStatistics.moveCount - instruction->candidateAttemptCount + instruction->schedulingStatistics.forwardLatencyDepth;
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
                    bestC = instruction->schedulingStatistics.reverseLatencyDepth;
                    best = instruction;
                }
            }
        }
    }
    if (!best)
        return false;
    c->candidateInstruction = best;
    c->candidateRow = best->candidateBaseRow + best->candidateCostOffset;
    c->candidatePipe = best->candidateAttemptCount & 1;
    return true;
}

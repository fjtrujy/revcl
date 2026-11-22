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


bool optAnother(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->moveListHead;
    int bestA = 0x80000000;
    Instr *best = 0;
    int bestB = 0x80000000, bestC = 0x80000000, bestD = 0x7fffffff;
    for (; in; in = in->nextMoveInstruction) {
        int cost = anotherRegCost(in);
        int a = in->schedulingStatistics.moveDistance - in->candidateAttemptCount + in->schedulingStatistics.reverseLatencyDepth + cost;
        if (a > bestA) {
            best = in;
            bestA = a;
            int b = in->schedulingStatistics.moveCount - in->candidateAttemptCount;
            int h = in->schedulingStatistics.forwardDependencyHeight;
            bestB = b + (h >> 2) + cost;
            bestC = in->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int b = in->schedulingStatistics.moveCount - in->candidateAttemptCount;
            int h = in->schedulingStatistics.forwardDependencyHeight;
            int e = b + (h >> 2) + cost;
            if (e > bestB) {
                bestB = e;
                bestC = in->schedulingStatistics.reverseLatencyDepth;
                best = in;
            } else if (e == bestB) {
                int f = in->candidateBaseRow + in->candidateCostOffset;
                if (f < bestD) {
                    bestD = f;
                    bestC = in->schedulingStatistics.reverseLatencyDepth;
                    best = in;
                } else if (f == bestD && in->schedulingStatistics.reverseDependencyHeight > bestC) {
                    bestC = in->schedulingStatistics.reverseDependencyHeight;
                    best = in;
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

int anotherRegCost(Instr *in)
{
    int cost = 0;
    for (int i = 0; i != in->operandCount; i++) {
        RegPoolEntry *r = in->operands[i].inRegOp2();
        if (r) {
            switch (r->registerIndex) {
            case 55:
            case 56:
            case 59:
                cost += 400;
            }
        }
    }
    return cost;
}

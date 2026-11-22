#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int normal2RegCost(Instr *in);
int vuta1RegCost(Instr *in);

bool optNormal2(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; in; in = in->nextMoveInstruction) {
        normal2RegCost(in);
        int d = in->schedulingStatistics.reverseLatencyDepth - in->candidateAttemptCount;
        if (d > bestA) {
            best = in;
            bestA = d;
            bestB = -in->linearOrderIndex;
            bestC = in->schedulingStatistics.reverseLatencyDepth;
        } else if (d == bestA) {
            int e = -in->linearOrderIndex;
            if (e > bestB) {
                bestB = e;
                bestC = in->schedulingStatistics.reverseLatencyDepth;
                best = in;
            } else if (e == bestB) {
                int f = in->candidateBaseRow + (in->candidateAttemptCount >> 2);
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
    c->candidateRow = (best->candidateAttemptCount >> 2) + best->candidateBaseRow;
    c->candidatePipe = (best->candidateAttemptCount >> 3) & 1;
    return true;
}

int normal2RegCost(Instr *in)
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

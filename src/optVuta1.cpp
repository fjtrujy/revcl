#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int vuta1RegCost(Instr *in);

bool optVuta1(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->moveListHead;
    int bestA = 0x80000000;
    Instr *best = 0;
    int bestB = 0x80000000, bestC = 0x80000000, bestD = 0x7fffffff;
    for (; in; in = in->nextMoveInstruction) {
        int cost = vuta1RegCost(in);
        int base = in->schedulingStatistics.moveDistance;
        int a = base - in->candidateAttemptCount + in->schedulingStatistics.forwardLatencyDepth * 32 + cost;
        if (a > bestA) {
            best = in;
            bestA = a;
            bestB = in->schedulingStatistics.moveCount - in->candidateAttemptCount + in->schedulingStatistics.forwardDependencyHeight + cost;
            bestC = in->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int e = in->schedulingStatistics.moveCount - in->candidateAttemptCount + in->schedulingStatistics.forwardDependencyHeight + cost;
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
                } else if (f == bestD && in->schedulingStatistics.reverseLatencyDepth > bestC) {
                    bestC = in->schedulingStatistics.reverseLatencyDepth;
                    best = in;
                }
            }
        }
    }
    if (!best)
        return false;
    c->unk_00 = best;
    c->unk_04 = (best->candidateAttemptCount >> 2) + best->candidateBaseRow;
    c->unk_06 = (best->candidateAttemptCount >> 3) & 1;
    return true;
}

int vuta1RegCost(Instr *in)
{
    int cost = 0;
    for (int i = 0; i != in->operandCount; i++) {
        RegPoolEntry *r = in->operands[i].inRegOp2();
        if (r && r->registerIndex == 59)
            cost += 400;
    }
    return cost;
}

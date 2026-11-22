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


bool optVuta(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; in; in = in->nextMoveInstruction) {
        int cost = vutaRegCost(in);
        int base = in->schedulingStatistics.moveDistance;
        int a = base - in->candidateAttemptCount + in->schedulingStatistics.forwardLatencyDepth * 32 + cost;
        if (a > bestA) {
            bestA = a;
            bestB = in->schedulingStatistics.moveCount - in->candidateAttemptCount + in->schedulingStatistics.forwardDependencyHeight + cost;
            best = in;
            bestC = in->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int e = in->schedulingStatistics.moveCount - in->candidateAttemptCount + in->schedulingStatistics.forwardDependencyHeight + cost;
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
    c->unk_04 = best->candidateBaseRow + best->candidateCostOffset;
    c->unk_06 = best->candidateAttemptCount & 1;
    return true;
}

int vutaRegCost(Instr *in)
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


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


bool optNormal1(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->moveListHead;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; in; in = in->nextMoveInstruction) {
        int a = in->schedulingStatistics.reverseLatencyDepth - in->candidateAttemptCount + in->schedulingStatistics.forwardLatencyDepth;
        if (a > bestA) {
            best = in;
            bestA = a;
            bestB = in->schedulingStatistics.moveDistance - in->candidateAttemptCount;
            bestC = in->schedulingStatistics.reverseLatencyDepth;
        } else if (a == bestA) {
            int e = in->schedulingStatistics.moveDistance - in->candidateAttemptCount;
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
    c->unk_00 = best;
    c->unk_04 = best->candidateBaseRow + best->candidateCostOffset;
    c->unk_06 = best->candidateAttemptCount & 1;
    return true;
}

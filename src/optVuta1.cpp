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
    for (; in; in = in->unk_2c) {
        int cost = vuta1RegCost(in);
        int base = in->unk_13f0.unk_0c;
        int a = base - in->unk_1418 + in->unk_13f0.unk_04 * 32 + cost;
        if (a > bestA) {
            best = in;
            bestA = a;
            bestB = in->unk_13f0.unk_08 - in->unk_1418 + in->unk_13f0.unk_06 + cost;
            bestC = in->unk_13f0.unk_00;
        } else if (a == bestA) {
            int e = in->unk_13f0.unk_08 - in->unk_1418 + in->unk_13f0.unk_06 + cost;
            if (e > bestB) {
                bestB = e;
                bestC = in->unk_13f0.unk_00;
                best = in;
            } else if (e == bestB) {
                int f = in->unk_141e + (in->unk_1418 >> 2);
                if (f < bestD) {
                    bestD = f;
                    bestC = in->unk_13f0.unk_00;
                    best = in;
                } else if (f == bestD && in->unk_13f0.unk_00 > bestC) {
                    bestC = in->unk_13f0.unk_00;
                    best = in;
                }
            }
        }
    }
    if (!best)
        return false;
    c->unk_00 = best;
    c->unk_04 = (best->unk_1418 >> 2) + best->unk_141e;
    c->unk_06 = (best->unk_1418 >> 3) & 1;
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

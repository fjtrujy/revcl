#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

int normal2RegCost(Instr *in);
int vuta1RegCost(Instr *in);

bool optNormal2(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->unk_4c;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; in; in = in->unk_2c) {
        normal2RegCost(in);
        int d = in->unk_13f0.unk_00 - in->unk_1418;
        if (d > bestA) {
            best = in;
            bestA = d;
            bestB = -in->unk_13e4;
            bestC = in->unk_13f0.unk_00;
        } else if (d == bestA) {
            int e = -in->unk_13e4;
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
                } else if (f == bestD && in->unk_13f0.unk_02 > bestC) {
                    bestC = in->unk_13f0.unk_02;
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

int normal2RegCost(Instr *in)
{
    int cost = 0;
    for (int i = 0; i != in->operandCount; i++) {
        RegPoolEntry *r = in->operands[i].inRegOp2();
        if (r) {
            switch (r->unk_30) {
            case 55:
            case 56:
            case 59:
                cost += 400;
            }
        }
    }
    return cost;
}

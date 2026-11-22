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
    Instr *in = wt->unk_4c;
    Instr *best = 0;
    int bestA = 0x80000000, bestB = 0x80000000, bestC = 0x80000000;
    int bestD = 0x7fffffff;
    for (; in; in = in->unk_2c) {
        int cost = vutaRegCost(in);
        int base = in->unk_13f0.unk_0c;
        int a = base - in->unk_1418 + in->unk_13f0.unk_04 * 32 + cost;
        if (a > bestA) {
            bestA = a;
            bestB = in->unk_13f0.unk_08 - in->unk_1418 + in->unk_13f0.unk_06 + cost;
            best = in;
            bestC = in->unk_13f0.unk_00;
        } else if (a == bestA) {
            int e = in->unk_13f0.unk_08 - in->unk_1418 + in->unk_13f0.unk_06 + cost;
            if (e > bestB) {
                bestB = e;
                bestC = in->unk_13f0.unk_00;
                best = in;
            } else if (e == bestB) {
                int f = in->unk_141e + in->unk_141c;
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
    c->unk_04 = best->unk_141e + best->unk_141c;
    c->unk_06 = best->unk_1418 & 1;
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


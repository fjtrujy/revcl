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
    Instr *in = wt->unk_4c;
    int bestA = 0x80000000;
    Instr *best = 0;
    int bestB = 0x80000000, bestC = 0x80000000, bestD = 0x7fffffff;
    for (; in; in = in->unk_2c) {
        int cost = anotherRegCost(in);
        int a = in->unk_13f0.unk_0c - in->unk_1418 + in->unk_13f0.unk_00 + cost;
        if (a > bestA) {
            best = in;
            bestA = a;
            int b = in->unk_13f0.unk_08 - in->unk_1418;
            int h = in->unk_13f0.unk_06;
            bestB = b + (h >> 2) + cost;
            bestC = in->unk_13f0.unk_00;
        } else if (a == bestA) {
            int b = in->unk_13f0.unk_08 - in->unk_1418;
            int h = in->unk_13f0.unk_06;
            int e = b + (h >> 2) + cost;
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
    c->unk_04 = best->unk_141e + best->unk_141c;
    c->unk_06 = best->unk_1418 & 1;
    return true;
}

int anotherRegCost(Instr *in)
{
    int cost = 0;
    for (int i = 0; i != in->unk_138b; i++) {
        RegPoolEntry *r = in->unk_38[i].inRegOp2();
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

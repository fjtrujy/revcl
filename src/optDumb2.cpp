#include "os.h"
#include "workTable.h"
#include "Instr.h"
#include "RegPool.h"
#include "wrangle.h"

bool optDumb2(workTable *wt, wrangleCand *c)
{
    Instr *in = wt->moveListHead;
    Instr *best = 0;
    int bestV = 0x7fffffff;
    for (; in; in = in->unk_2c) {
        if (in->linearOrderIndex < bestV) {
            best = in;
            bestV = in->linearOrderIndex;
        }
    }
    if (!best)
        return false;
    c->unk_00 = best;
    c->unk_04 = (best->unk_1418 >> 1) + best->unk_141e;
    if (!best->unk_1428) {
        int row = wt->unk_54.getRow();
        if (c->unk_04 < (short)row)
            c->unk_04 = row;
    }
    c->unk_06 = best->unk_1418 & 1;
    return true;
}
